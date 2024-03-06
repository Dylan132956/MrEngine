@echo 删除当前目录和子目录的所有.git的文件和文件夹
@for /r . %%a in (.) do @if exist "%%a/.git" rd /s /q "%%a/.git"
@echo 已经全部删除
@pause
