# Default KataGo Neural Model

- File: `tools/b10c384h6nbttflrs.bin.gz`
- Source: [Official KataGo v1.17.1 release](https://github.com/lightvector/KataGo/releases/tag/v1.17.1)
- [Direct download](https://github.com/lightvector/KataGo/releases/download/v1.17.1/b10c384h6nbttflrs.bin.gz)
- Compressed size: 38,245,488 bytes
- SHA-256: `0ba27eced5180b3e3d0b898b280c541112989765e789d1eb6cd0d31b2b2c1229`
- Type: playing transformer, 10 blocks, 384 channels, 6 attention heads.

This is the smallest playing transformer provided in that official release. It was selected to reduce inference cost while retaining a neural playing model. It is not a HumanSL profile model and is not presented as the newest available network.

The compressed model is ignored by Git. The launcher uses it by default:

```bash
./tools/run-katago-hard.sh --ai-time-ms 1800 --ai-threads 4 --katago-duty 0.30
```

Hard keeps GoGame's own MCTS and loads the neural weights through the native `KataGoEvaluator`. The standalone KataGo executable is not used. See [backend setup](../docs/ai/KATAGO_BACKEND.md) for CUDA/cuDNN dependencies or the CPU/Eigen path.

To download it again:

```bash
curl -fL --retry 2 \
  https://github.com/lightvector/KataGo/releases/download/v1.17.1/b10c384h6nbttflrs.bin.gz \
  -o tools/b10c384h6nbttflrs.bin.gz.part &&
echo '0ba27eced5180b3e3d0b898b280c541112989765e789d1eb6cd0d31b2b2c1229  tools/b10c384h6nbttflrs.bin.gz.part' | sha256sum --check &&
gzip --test tools/b10c384h6nbttflrs.bin.gz.part &&
mv tools/b10c384h6nbttflrs.bin.gz.part tools/b10c384h6nbttflrs.bin.gz
```

Only rename the partial file after both checks pass. Supply a positional model path or `GOGAME_KATAGO_MODEL` to use different weights.
