#!/usr/bin/env python3
"""SGF -> supervised policy/value -> 6B transformer or small ResNet -> checkpoint/ONNX.
SGF preparation uses the standard library; training imports PyTorch lazily.
"""
from __future__ import annotations
import argparse
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import random
import sys

SIZE = 19
PASS = SIZE * SIZE
PLANES = 5
BLOCKS = 6
TRANSFORMER_6B = {"width": 4096, "hidden": 9728, "layers": 32, "heads": 32}
NEIGHBORS = tuple(tuple(q for q in (p - SIZE if p // SIZE else -1,
                                   p + SIZE if p // SIZE < SIZE - 1 else -1,
                                   p - 1 if p % SIZE else -1,
                                   p + 1 if p % SIZE < SIZE - 1 else -1) if q >= 0)
                  for p in range(PASS))


@dataclass
class Tree:
    nodes: list[dict[str, list[str]]]
    children: list[Tree]

    def mainline(self):
        yield from self.nodes
        if self.children:
            yield from self.children[0].mainline()


class SGFParser:
    def __init__(self, text: str):
        self.text = text.lstrip("\ufeff")
        self.pos = 0

    def whitespace(self):
        while self.pos < len(self.text) and self.text[self.pos].isspace():
            self.pos += 1

    def peek(self):
        self.whitespace()
        return self.text[self.pos:self.pos + 1]

    def expect(self, token):
        if self.peek() != token:
            raise ValueError(f"Expected {token!r} at character {self.pos}")
        self.pos += 1

    def value(self):
        self.expect("[")
        out = []
        while self.pos < len(self.text):
            char = self.text[self.pos]
            self.pos += 1
            if char == "]":
                return "".join(out)
            if char == "\\":
                if self.pos >= len(self.text):
                    break
                char = self.text[self.pos]
                self.pos += 1
                if char == "\r":
                    if self.text[self.pos:self.pos + 1] == "\n":
                        self.pos += 1
                    continue
                if char == "\n":
                    continue
            out.append(char)
        raise ValueError("Unterminated SGF property value")

    def tree(self, depth=0):
        if depth > 128:
            raise ValueError("SGF variation nesting is too deep")
        self.expect("(")
        nodes = []
        while self.peek() == ";":
            self.pos += 1
            node = {}
            while self.peek() and self.peek().isalpha():
                start = self.pos
                while self.pos < len(self.text) and self.text[self.pos].isalpha():
                    self.pos += 1
                key = self.text[start:self.pos]
                if not key.isupper() or key in node:
                    raise ValueError(f"Invalid or repeated property {key}")
                values = []
                while self.peek() == "[":
                    values.append(self.value())
                if not values:
                    raise ValueError(f"Missing value for {key}")
                node[key] = values
            nodes.append(node)
        if not nodes:
            raise ValueError("Empty SGF game tree")
        children = []
        while self.peek() == "(":
            children.append(self.tree(depth + 1))
        self.expect(")")
        return Tree(nodes, children)

    def collection(self):
        result = []
        while self.peek():
            result.append(self.tree())
        if not result:
            raise ValueError("Empty SGF file")
        return result


def point(value: str) -> int:
    if len(value) != 2 or not all("a" <= char <= "s" for char in value):
        raise ValueError(f"Invalid 19x19 SGF coordinate {value!r}")
    # SGF is column,row; runtime policy is row-major: row*19+col.
    return (ord(value[1]) - 97) * SIZE + ord(value[0]) - 97


def setup_points(value):
    if ":" not in value:
        return [point(value)]
    first, last = map(point, value.split(":"))
    r1, c1 = divmod(first, SIZE)
    r2, c2 = divmod(last, SIZE)
    if r2 < r1 or c2 < c1:
        raise ValueError("Inverted setup rectangle")
    return [row * SIZE + col for row in range(r1, r2 + 1) for col in range(c1, c2 + 1)]


class Board:
    def __init__(self):
        self.cells = [-1] * PASS
        self.previous = None
        self.turn = 0

    def candidate(self, policy):
        if policy == PASS:
            return self.cells[:]
        if not 0 <= policy < PASS or self.cells[policy] != -1:
            raise ValueError("Occupied or outside board")
        cells = self.cells[:]
        cells[policy] = self.turn

        def group(start):
            queue, seen, liberties = [start], {start}, set()
            for cell in queue:
                for neighbor in NEIGHBORS[cell]:
                    if cells[neighbor] == -1:
                        liberties.add(neighbor)
                    elif cells[neighbor] == cells[start] and neighbor not in seen:
                        seen.add(neighbor)
                        queue.append(neighbor)
            return queue, liberties

        for neighbor in NEIGHBORS[policy]:
            if cells[neighbor] == 1 - self.turn:
                stones, liberties = group(neighbor)
                if not liberties:
                    for stone in stones:
                        cells[stone] = -1
        if not group(policy)[1]:
            raise ValueError("Suicide")
        if self.previous is not None and tuple(cells) == self.previous:
            raise ValueError("Simple ko")
        return cells

    def play(self, policy):
        cells = self.candidate(policy)
        self.previous = tuple(self.cells)
        self.cells = cells
        self.turn = 1 - self.turn

    def legal_mask(self):
        mask = []
        for policy in range(PASS):
            if self.cells[policy] != -1:
                mask.append(0)
                continue
            try:
                self.candidate(policy)
                mask.append(1)
            except ValueError:
                mask.append(0)
        return mask + [1]

    def setup(self, node):
        assigned = set()
        for key, color in (("AB", 0), ("AW", 1), ("AE", -1)):
            for value in node.get(key, []):
                for cell in setup_points(value):
                    if cell in assigned:
                        raise ValueError("Conflicting setup properties")
                    assigned.add(cell)
                    self.cells[cell] = color
        if assigned:
            self.previous = None
        if "PL" in node:
            if node["PL"] not in (["B"], ["W"]):
                raise ValueError("Invalid PL")
            self.turn = int(node["PL"][0] == "W")


def outcome(result):
    result = result.strip()
    if result.lower() in ("0", "draw", "jigo"):
        return 0
    if len(result) > 2 and result[0] in "BW" and result[1] == "+":
        return 1 if result[0] == "B" else -1
    raise ValueError("Missing/unknown RE: cannot create value target")


def samples(tree: Tree, include_legal=True):
    nodes = list(tree.mainline())
    root = nodes[0]
    if root.get("GM", ["1"]) != ["1"] or root.get("SZ", ["19"]) != ["19"]:
        raise ValueError("Only Go on a 19x19 board is supported")
    winner = outcome(root.get("RE", [""])[0])
    komi = float(root.get("KM", ["0"])[0])
    if not math.isfinite(komi):
        raise ValueError("Non-finite komi")
    board = Board()
    if int(root.get("HA", ["0"])[0]) >= 2:
        board.turn = 1
    records = []
    for node in nodes:
        if ("B" in node or "W" in node) and any(key in node for key in ("AB", "AW", "AE")):
            raise ValueError("Setup and move cannot share an SGF node")
        board.setup(node)
        moves = [key for key in ("B", "W") if key in node]
        if not moves:
            continue
        if len(moves) != 1 or len(node[moves[0]]) != 1:
            raise ValueError("Multiple moves in one node")
        color = int(moves[0] == "W")
        if color != board.turn:
            raise ValueError("Move color disagrees with player to play")
        value = node[moves[0]][0]
        policy = PASS if value in ("", "tt") else point(value)
        record = {"board": board.cells[:], "turn": board.turn, "komi": komi,
                  "policy": policy, "value": winner if board.turn == 0 else -winner}
        if include_legal:
            record["legal"] = board.legal_mask()
            if not record["legal"][policy]:
                raise ValueError("SGF target is not a legal move")
        board.play(policy)  # Validate the complete game before writing any samples.
        records.append(record)
        if len(records) > 5000:
            raise ValueError("SGF exceeds 5000 moves")
    if not records:
        raise ValueError("Game has no moves")
    canonical = hashlib.sha256()
    canonical.update(f"{komi.hex()}:{winner}".encode())
    for record in records:
        canonical.update(bytes(cell + 1 for cell in record["board"]))
        canonical.update(bytes([record["turn"]]))
        canonical.update(record["policy"].to_bytes(2, "big"))
    game_id = canonical.hexdigest()
    for record in records:
        record["game_id"] = game_id
    return records


def sgf_games(directory, include_legal=True):
    paths = sorted(path for path in Path(directory).rglob("*") if path.suffix.lower() == ".sgf")
    if not paths:
        raise ValueError("No .sgf files found")
    for path in paths:
        try:
            text = path.read_bytes().decode("utf-8", errors="replace")
            trees = SGFParser(text).collection()
        except (ValueError, RecursionError) as error:
            yield path, None, str(error)
            continue
        for tree in trees:
            try:
                yield path, samples(tree, include_legal), None
            except (ValueError, RecursionError) as error:
                yield path, None, str(error)


def prepare(directory, output=None, validation_fraction=0.1, seed=42):
    if not 0 < validation_fraction < 1:
        raise ValueError("Validation fraction must be between 0 and 1")
    counts = {"games": 0, "positions": 0, "skipped": 0, "duplicates": 0,
              "train_games": 0, "validation_games": 0, "train_positions": 0, "validation_positions": 0}
    seen, files = set(), {}
    try:
        if output:
            Path(output).mkdir(parents=True, exist_ok=True)
            files = {name: (Path(output) / f"{name}.jsonl").open("w", encoding="utf-8")
                     for name in ("train", "validation")}
        for path, records, error in sgf_games(directory, include_legal=bool(output)):
            if error:
                counts["skipped"] += 1
                print(f"Skipped {path.name}: {error}", file=sys.stderr)
                continue
            identity = records[0]["game_id"]
            if identity in seen:
                counts["duplicates"] += 1
                continue
            seen.add(identity)
            fraction = int(hashlib.sha256(f"{seed}:{identity}".encode()).hexdigest()[:8], 16) / 2**32
            split = "validation" if fraction < validation_fraction else "train"
            counts["games"] += 1
            counts["positions"] += len(records)
            counts[f"{split}_games"] += 1
            counts[f"{split}_positions"] += len(records)
            if output:
                for record in records:
                    files[split].write(json.dumps(record, separators=(",", ":")) + "\n")
    finally:
        for file in files.values():
            file.close()
    if not counts["games"]:
        raise ValueError("No valid labeled games found")
    manifest = {"format_version": 1, "board_size": SIZE, "input_planes": PLANES,
                "policy_size": PASS + 1, "seed": seed,
                "validation_fraction": validation_fraction, "counts": counts}
    if output:
        (Path(output) / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


def require_torch():
    try:
        import torch
        return torch
    except ImportError as error:
        raise RuntimeError("Install src/AI/training/requirements.txt before training/export") from error


def transformer_parameter_count(config=None):
    """Count trainable parameters without importing torch or allocating weights."""
    config = config or TRANSFORMER_6B
    d, f, layers = config["width"], config["hidden"], config["layers"]
    # QKV/output, SwiGLU's three projections, two RMS norms per block.
    return layers * (4*d*d + 3*d*f + 2*d) + (PLANES+1)*d + PASS*d + d + 3*(d+1)


def build_transformer(config=None):
    torch = require_torch()
    nn = torch.nn
    config = config or TRANSFORMER_6B
    d, f, layers, heads = (config[k] for k in ("width", "hidden", "layers", "heads"))
    if d % heads or min(d, f, layers, heads) < 1:
        raise ValueError("Invalid transformer dimensions")

    class Block(nn.Module):
        def __init__(self):
            super().__init__()
            self.norm1, self.norm2 = nn.RMSNorm(d, eps=1e-6), nn.RMSNorm(d, eps=1e-6)
            self.qkv, self.out = nn.Linear(d, 3*d, bias=False), nn.Linear(d, d, bias=False)
            self.gate, self.up = nn.Linear(d, f, bias=False), nn.Linear(d, f, bias=False)
            self.down = nn.Linear(f, d, bias=False)
        def forward(self, x):
            qkv = self.qkv(self.norm1(x)).reshape(x.shape[0], PASS, 3, heads, d//heads)
            q, k, v = qkv.permute(2, 0, 3, 1, 4).unbind(0)
            attention = torch.nn.functional.scaled_dot_product_attention(q, k, v, dropout_p=0.0)
            x = x + self.out(attention.transpose(1, 2).reshape(x.shape[0], PASS, d))
            normalized = self.norm2(x)
            return x + self.down(torch.nn.functional.silu(self.gate(normalized)) * self.up(normalized))

    class GoTransformer(nn.Module):
        def __init__(self):
            super().__init__()
            self.stem = nn.Linear(PLANES, d)
            self.position = nn.Parameter(torch.empty(1, PASS, d))
            nn.init.normal_(self.position, std=0.02)
            self.blocks = nn.Sequential(*(Block() for _ in range(layers)))
            self.norm = nn.RMSNorm(d, eps=1e-6)
            self.policy, self.pass_head, self.value = nn.Linear(d, 1), nn.Linear(d, 1), nn.Linear(d, 1)
        def forward(self, planes):
            x = self.stem(planes.flatten(2).transpose(1, 2)) + self.position
            x = self.norm(self.blocks(x))
            pooled = x.mean(1)
            policy = torch.cat((self.policy(x).squeeze(-1), self.pass_head(pooled)), dim=1)
            return policy, torch.tanh(self.value(pooled))
    return GoTransformer()


def build_model(channels=64, architecture="resnet6"):
    if architecture == "6b":
        return build_transformer()
    if architecture != "resnet6":
        raise ValueError("Unknown model architecture")
    torch = require_torch()
    nn = torch.nn

    class ResidualBlock(nn.Module):
        def __init__(self):
            super().__init__()
            self.layers = nn.Sequential(nn.Conv2d(channels, channels, 3, padding=1, bias=False),
                nn.BatchNorm2d(channels), nn.ReLU(), nn.Conv2d(channels, channels, 3, padding=1, bias=False), nn.BatchNorm2d(channels))
        def forward(self, planes):
            return torch.relu(planes + self.layers(planes))

    class GoResNet6(nn.Module):
        def __init__(self):
            super().__init__()
            self.stem = nn.Sequential(nn.Conv2d(PLANES, channels, 3, padding=1, bias=False), nn.BatchNorm2d(channels), nn.ReLU())
            self.blocks = nn.Sequential(*(ResidualBlock() for _ in range(BLOCKS)))
            self.policy = nn.Sequential(nn.Conv2d(channels, 2, 1), nn.ReLU(), nn.Flatten(), nn.Linear(2 * PASS, PASS + 1))
            self.value = nn.Sequential(nn.Conv2d(channels, 1, 1), nn.ReLU(), nn.Flatten(), nn.Linear(PASS, channels), nn.ReLU(), nn.Linear(channels, 1), nn.Tanh())
        def forward(self, planes):
            features = self.blocks(self.stem(planes))
            return self.policy(features), self.value(features)
    return GoResNet6()


def make_loader(path, batch_size, augment=False):
    torch = require_torch()

    class Records(torch.utils.data.IterableDataset):
        def __iter__(self):
            with Path(path).open(encoding="utf-8") as stream:
                for line in stream:
                    record = json.loads(line)
                    board = torch.tensor(record["board"]).reshape(SIZE, SIZE)
                    turn = record["turn"]
                    legal = torch.tensor(record["legal"], dtype=torch.bool)
                    planes = torch.stack(((board == turn).float(), (board == 1-turn).float(),
                        legal[:PASS].reshape(SIZE, SIZE).float(), torch.full((SIZE, SIZE), float(turn == 0)),
                        torch.full((SIZE, SIZE), record["komi"] * (1 if turn == 1 else -1) / 20)))
                    policy = record["policy"]
                    if augment:
                        rotation, flip = random.randrange(4), bool(random.randrange(2))
                        planes = torch.rot90(planes, rotation, dims=(1, 2))
                        grid = legal[:PASS].reshape(SIZE, SIZE)
                        grid = torch.rot90(grid, rotation, dims=(0, 1))
                        target = torch.zeros(SIZE, SIZE)
                        if policy < PASS:
                            target[policy // SIZE, policy % SIZE] = 1
                        target = torch.rot90(target, rotation, dims=(0, 1))
                        if flip:
                            planes, grid, target = planes.flip(2), grid.flip(1), target.flip(1)
                        legal = torch.cat((grid.reshape(-1), legal[PASS:]))
                        if policy < PASS:
                            policy = int(target.reshape(-1).argmax())
                    yield planes, legal, policy, float(record["value"])
    return torch.utils.data.DataLoader(Records(), batch_size=batch_size, num_workers=0)


def train(data_dir, output, epochs=10, batch_size=128, channels=64, threads=3, resume=None, seed=42, architecture="6b"):
    torch = require_torch()
    torch.set_num_threads(threads)
    random.seed(seed)
    torch.manual_seed(seed)
    manifest = json.loads((Path(data_dir) / "manifest.json").read_text())
    if manifest.get("format_version") != 1 or manifest.get("input_planes") != PLANES:
        raise ValueError("Unsupported prepared dataset contract")
    counts = manifest["counts"]
    if not counts["train_positions"] or not counts["validation_positions"]:
        raise ValueError("Need both train and validation games; add SGFs or change preparation seed/split")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    checkpoint = torch.load(resume, map_location="cpu", weights_only=True) if resume else None
    if checkpoint and (checkpoint["input_planes"] != PLANES or checkpoint["board_size"] != SIZE or checkpoint["policy_size"] != PASS + 1):
        raise ValueError("Checkpoint architecture mismatch")
    if checkpoint:
        channels = checkpoint["channels"]
        architecture = checkpoint.get("architecture", "resnet6")
        if (architecture == "6b" and checkpoint.get("transformer_config") != TRANSFORMER_6B) or (architecture == "resnet6" and checkpoint["blocks"] != BLOCKS):
            raise ValueError("Checkpoint architecture mismatch")
    model = build_model(channels, architecture).to(device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=1e-3, weight_decay=1e-4)
    first_epoch = 0
    if checkpoint:
        model.load_state_dict(checkpoint["model"])
        optimizer.load_state_dict(checkpoint["optimizer"])
        first_epoch = checkpoint["epoch"]
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    for epoch in range(first_epoch, first_epoch + epochs):
        metrics = {}
        for split in ("train", "validation"):
            model.train(split == "train")
            total_loss, correct, count = 0.0, 0, 0
            for planes, legal, target, value in make_loader(Path(data_dir) / f"{split}.jsonl", batch_size, split == "train"):
                planes, legal, target, value = planes.to(device), legal.to(device), target.to(device), value.float().to(device)
                with torch.set_grad_enabled(split == "train"):
                    logits, predicted = model(planes)
                    logits = logits.masked_fill(~legal, -1e9)
                    loss = torch.nn.functional.cross_entropy(logits, target) + torch.nn.functional.mse_loss(predicted[:, 0], value)
                    if not torch.isfinite(loss):
                        raise RuntimeError("Non-finite training loss; checkpoint was not replaced")
                    if split == "train":
                        optimizer.zero_grad(set_to_none=True)
                        loss.backward()
                        optimizer.step()
                count += len(target)
                total_loss += float(loss.detach()) * len(target)
                correct += int((logits.argmax(1) == target).sum())
            if not count:
                raise ValueError(f"Empty {split} stream")
            metrics[split] = {"loss": total_loss/count, "policy_accuracy": correct/count}
        temporary = Path(str(output) + ".tmp")
        torch.save({"model": model.state_dict(), "optimizer": optimizer.state_dict(), "epoch": epoch + 1,
                    "architecture": architecture, "transformer_config": TRANSFORMER_6B if architecture == "6b" else None,
                    "channels": channels, "blocks": TRANSFORMER_6B["layers"] if architecture == "6b" else BLOCKS, "input_planes": PLANES,
                    "board_size": SIZE, "policy_size": PASS + 1, "metrics": metrics}, temporary)
        temporary.replace(output)
        print(json.dumps({"epoch": epoch + 1, "device": str(device), "metrics": metrics}))


def export_model(checkpoint_path, output):
    torch = require_torch()
    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=True)
    if checkpoint["input_planes"] != PLANES or checkpoint["board_size"] != SIZE or checkpoint["policy_size"] != PASS + 1:
        raise ValueError("Checkpoint architecture mismatch")
    architecture = checkpoint.get("architecture", "resnet6")
    if (architecture == "6b" and checkpoint.get("transformer_config") != TRANSFORMER_6B) or (architecture == "resnet6" and checkpoint["blocks"] != BLOCKS):
        raise ValueError("Checkpoint architecture mismatch")
    model = build_model(checkpoint["channels"], architecture)
    model.load_state_dict(checkpoint["model"])
    model.eval()
    example = torch.zeros(2, PLANES, SIZE, SIZE)  # Avoid specializing symbolic batch to 1.
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    torch.onnx.export(model, (example,), str(output), input_names=["planes"],
        output_names=["policy_logits", "value"], opset_version=18, dynamo=True,
        dynamic_shapes={"planes": {0: torch.export.Dim("batch", min=1)}}, external_data=architecture == "6b")
    contract = {"board_size": SIZE, "blocks": checkpoint["blocks"], "channels": checkpoint["channels"],
                "architecture": architecture, "transformer_config": TRANSFORMER_6B if architecture == "6b" else None,
                "input": "planes", "input_shape": ["batch", PLANES, SIZE, SIZE],
                "planes": ["own_stones", "opponent_stones", "legal_moves", "black_to_play", "signed_komi_div_20"],
                "policy_output": "policy_logits", "policy_shape": ["batch", PASS + 1],
                "pass_index": PASS, "value_output": "value", "value_shape": ["batch", 1],
                "value_perspective": "player_to_move", "epoch": checkpoint["epoch"]}
    Path(str(output) + ".json").write_text(json.dumps(contract, indent=2))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--inspect", metavar="SGF_DIR")
    modes.add_argument("--prepare", metavar="SGF_DIR")
    modes.add_argument("--train", metavar="PREPARED_DIR")
    modes.add_argument("--export", metavar="CHECKPOINT_PT")
    modes.add_argument("--describe-model", action="store_true", help="Print config/parameter count without allocating a model")
    parser.add_argument("--output", help="Prepared directory, .pt checkpoint or .onnx file")
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--batch-size", type=int, default=1)
    parser.add_argument("--channels", type=int, default=64)
    parser.add_argument("--architecture", choices=("6b", "resnet6"), default="6b")
    parser.add_argument("--threads", type=int, default=3)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--validation-fraction", type=float, default=0.1)
    parser.add_argument("--resume", metavar="CHECKPOINT_PT")
    args = parser.parse_args(argv)
    if not args.inspect and not args.describe_model and not args.output:
        parser.error("--output is required")
    if args.epochs < 1 or args.batch_size < 1 or args.channels < 8 or not 1 <= args.threads <= 3:
        parser.error("epochs/batch-size must be positive, channels >= 8, threads 1..3")
    try:
        if args.describe_model:
            print(json.dumps({"architecture": args.architecture, "config": TRANSFORMER_6B if args.architecture == "6b" else {"blocks": BLOCKS, "channels": args.channels},
                              "parameters": transformer_parameter_count() if args.architecture == "6b" else None}, indent=2))
        elif args.inspect or args.prepare:
            print(json.dumps(prepare(args.inspect or args.prepare, args.output if args.prepare else None,
                                     args.validation_fraction, args.seed), indent=2))
        elif args.train:
            train(args.train, args.output, args.epochs, args.batch_size, args.channels, args.threads, args.resume, args.seed, args.architecture)
        else:
            export_model(args.export, args.output)
    except (ValueError, RuntimeError, OSError, KeyError, ImportError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
