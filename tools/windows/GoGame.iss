#ifndef PackageDir
  #define PackageDir "..\..\out\deploy\GoGame-KataGo-Release"
#endif

#define ModelPath "{app}\models\b10c384h6nbttflrs.bin.gz"

[Setup]
AppId=GoGame.KataGo
AppName=GoGame
AppVersion=0.1
DefaultDirName={autopf}\GoGame
DefaultGroupName=GoGame
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile=GoGame-License.txt
OutputDir=..\..\out\deploy
OutputBaseFilename=gogame_installer
UninstallDisplayIcon={app}\appGoGame.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=gogame.ico

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "pinhelp"; Description: "Show how to pin GoGame to the taskbar"; Flags: unchecked

[Files]
Source: "{#PackageDir}\prerequisites\vc_redist.x64.exe"; Flags: dontcopy
Source: "{#PackageDir}\*"; DestDir: "{app}"; Excludes: "prerequisites\*,Run-GoGame.cmd"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GoGame"; Filename: "{app}\appGoGame.exe"; Parameters: "--katago-model ""{#ModelPath}"""; WorkingDir: "{app}"
Name: "{autodesktop}\GoGame"; Filename: "{app}\appGoGame.exe"; Parameters: "--katago-model ""{#ModelPath}"""; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\appGoGame.exe"; Parameters: "--katago-model ""{#ModelPath}"""; WorkingDir: "{app}"; Description: "Launch GoGame"; Flags: nowait postinstall skipifsilent

[Code]
function HasRequiredRuntime: Boolean;
var
  Installed, Major, Minor, Build: Cardinal;
  Key: String;
begin
  Key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  Result := RegQueryDWordValue(HKLM64, Key, 'Installed', Installed) and
    (Installed = 1) and RegQueryDWordValue(HKLM64, Key, 'Major', Major) and
    RegQueryDWordValue(HKLM64, Key, 'Minor', Minor) and
    RegQueryDWordValue(HKLM64, Key, 'Bld', Build) and
    ((Major > 14) or ((Major = 14) and
    ((Minor > 52) or ((Minor = 52) and (Build >= 37029)))));
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
  Parameters: String;
begin
  Result := '';
  if HasRequiredRuntime then
    Exit;

  ExtractTemporaryFile('vc_redist.x64.exe');
  if WizardSilent then
    Parameters := '/install /quiet /norestart'
  else
    Parameters := '/install /passive /norestart';

  if not Exec(ExpandConstant('{tmp}\vc_redist.x64.exe'), Parameters, '',
    SW_SHOW, ewWaitUntilTerminated, ResultCode) then
    Result := 'Could not start the Visual C++ Redistributable installer: ' +
      SysErrorMessage(ResultCode)
  else if (ResultCode = 3010) or (ResultCode = 1641) then
  begin
    NeedsRestart := True;
    Result := 'Restart Windows and run GoGame Setup again to finish installation.';
  end
  else if not HasRequiredRuntime then
    Result := Format('Visual C++ Redistributable installation did not complete (exit code %d).', [ResultCode]);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssDone) and not WizardSilent and WizardIsTaskSelected('pinhelp') then
    MsgBox('Find GoGame in the Start menu, right-click it, and choose More > Pin to taskbar.',
      mbInformation, MB_OK);
end;
