Rem ����ΪVbScript�ű�
Set WshShell = WScript.CreateObject("WScript.Shell")
strDesktop = WshShell.SpecialFolders("Desktop") :'�����ļ��С����桱
Rem �����洴��һ�����±���ݷ�ʽ
set oShellLink = WshShell.CreateShortcut(strDesktop & "\SV�ļ�����.lnk")
oShellLink.TargetPath = "C:\Program Files (x86)\file_search\search_file.exe" : 'Ŀ��
oShellLink.WindowStyle = 3 :'����1Ĭ�ϴ��ڼ������3��󻯼������7��С��

oShellLink.IconLocation = "C:\Program Files (x86)\file_search\search_file.exe"'ͼ��
oShellLink.Description = "������ݷ�ʽ" : '��ע
oShellLink.WorkingDirectory = strDesktop : '��ʼλ��
oShellLink.Save : '���������ݷ�ʽ