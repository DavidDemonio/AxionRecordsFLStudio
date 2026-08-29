#define MyAppName "SteamMic Send"
#define MyAppVersion "1.0.0"
#define MyPublisher "SteamMic Tools"

[Setup]
AppId={{7F9B0993-5B5B-45A8-A0D0-B3EC4E6DC8FD}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyPublisher}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=dist
OutputBaseFilename=SteamMicSend-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}

[Files]
Source: "package\SteamMicSend.vst3\*"; DestDir: "{commoncf64}\VST3\SteamMicSend.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\SteamMicSend.vst3"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    MsgBox('Instalado. En FL Studio: Options > Manage plugins > Find installed plugins. Inserta "SteamMic Send" en el canal que quieras enviar. En Discord selecciona "Steam Streaming Microphone" como entrada.', mbInformation, MB_OK);
end;
