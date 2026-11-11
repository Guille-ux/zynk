[Setup]
AppName=Zynk
AppVersion=1.8.1
AppPublisher=Guille
DefaultDirName={localappdata}\Zynk
DefaultGroupName=Zynk
OutputDir=installer_output
OutputBaseFilename=Zynk_Setup_v1.0.0
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=lowest

[Files]
Source: "release\zynk.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Zynk"; Filename: "{app}\zynk.exe"

[Code]
const
  MY_HWND_BROADCAST = $FFFF;
  MY_WM_SETTINGCHANGE = $001A;
  SMTO_ABORTIFHUNG = $0002;

function SendMessageTimeoutW(hWnd: HWND; Msg: UINT; wParam: LongInt; lParam: String; fuFlags: UINT; uTimeout: UINT; var lpdwResult: DWORD): LongINT;
external 'SendMessageTimeoutW@user32.dll stdcall';

procedure CurStepChanged(CurStep: TSetupStep);
var
  OldPath: String;
  NewPath: String;
  AppDir: String;
  ResultCode: DWORD;
begin
  if CurStep = ssPostInstall then
  begin
    AppDir := ExpandConstant('{app}');
    
    if RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OldPath) then
    begin
      if Pos(';' + AppDir + ';', ';' + OldPath + ';') = 0 then
      begin
        NewPath := OldPath + ';' + AppDir;
        RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', NewPath);
      end;
    end
    else
    begin
      RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', AppDir);
    end;

    SendMessageTimeoutW(MY_HWND_BROADCAST, MY_WM_SETTINGCHANGE, 0, 'Environment', SMTO_ABORTIFHUNG, 5000, ResultCode);
  end;
end;
