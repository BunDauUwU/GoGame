function Component() {}

Component.prototype.createOperations = function() {
    component.createOperations();
    if (systemInfo.productType === "windows") {
        var model = installer.toNativeSeparators(installer.value("TargetDir") + "/models/model.bin.gz");
        var args = '--katago-model "' + model + '"';
        component.addOperation("CreateShortcut", "@TargetDir@/appGoGame.exe",
                               "@StartMenuDir@/GoGame.lnk", args);
        component.addOperation("CreateShortcut", "@TargetDir@/appGoGame.exe",
                               "@DesktopDir@/GoGame.lnk", args);
    } else {
        component.addOperation("CreateDesktopEntry", "gogame.desktop",
            "Type=Application\nName=GoGame\nComment=Play Go with KataGo AI\n"
            + 'Exec="@TargetDir@/run-gogame.sh"\n'
            + "Icon=@TargetDir@/gogame.png\nTerminal=false\nCategories=Game;BoardGame;\n");
    }
};
