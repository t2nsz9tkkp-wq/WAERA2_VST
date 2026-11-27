[Setup]
AppName=WAERA-2 Synthesizer
AppVersion=2.0.0
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
OutputDir=..\dist
OutputBaseFilename=WAERA-2_Windows_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "..\..\build\WAERA2_artefacts\Release\VST3\WAERA-2.vst3\*"; DestDir: "{commoncf64}\VST3\WAERA-2.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\Banks\*"; DestDir: "{userdocs}\WAERA-2\Banks"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\Skins\*"; DestDir: "{userdocs}\WAERA-2\Skins"; Flags: ignoreversion recursesubdirs createallsubdirs
