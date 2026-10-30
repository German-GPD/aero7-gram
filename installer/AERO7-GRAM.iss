#define MyAppName "AERO7 GRAM"
#define MyAppVersion "0.2.0-test"
#define MyAppPublisher "German'$ G-Code Team"
#define MyAppURL "https://german-gpd.github.io/aero7-gram/"
#define MyAppExeName "AERO7 GRAM.exe"

[Setup]
AppId={{B4D4E7C1-7F13-4E58-A707-7B1C8C4F7A20}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL=https://github.com/German-GPD/aero7-gram/issues
AppUpdatesURL=https://github.com/German-GPD/aero7-gram/releases
DefaultDirName={autopf}\German\G-Code-Team\Aero7Gram
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
OutputDir=.
OutputBaseFilename=AERO7-GRAM-{#AppArch}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}
CloseApplications=yes
RestartApplications=no
Uninstallable=yes
VersionInfoVersion=0.2.0.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=AERO7 GRAM test installer

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"; GroupDescription: "Дополнительные задачи:"; Flags: unchecked

[Files]
Source: "build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\AERO7 GRAM"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\AERO7 GRAM"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Запустить AERO7 GRAM"; Flags: postinstall nowait skipifsilent
