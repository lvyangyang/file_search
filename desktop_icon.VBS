Rem 以下为VbScript脚本
Set WshShell = WScript.CreateObject("WScript.Shell")
strDesktop = WshShell.SpecialFolders("Desktop") :'特殊文件夹“桌面”
Rem 在桌面创建一个记事本快捷方式
set oShellLink = WshShell.CreateShortcut(strDesktop & "\SV文件搜索.lnk")
oShellLink.TargetPath = "C:\Program Files (x86)\file_search\search_file.exe" : '目标
oShellLink.WindowStyle = 3 :'参数1默认窗口激活，参数3最大化激活，参数7最小化

oShellLink.IconLocation = "C:\Program Files (x86)\file_search\search_file.exe"'图标
oShellLink.Description = "搜索快捷方式" : '备注
oShellLink.WorkingDirectory = strDesktop : '起始位置
oShellLink.Save : '创建保存快捷方式