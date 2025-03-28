@echo off
setlocal

:: Set SQLite database and CSV file path
set "DB=ResultSet.db3"
set "SQLITE_EXE=sqlite3.exe"


:: Get current date and time
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set CURRENTDATE=%datetime:~0,4%-%datetime:~4,2%-%datetime:~6,2% %datetime:~8,2%:%datetime:~10,2%:%datetime:~12,2%

:: Get the latest NSE_FO_contract file
for /f "delims=" %%F in ('dir /b /o-d NSE_FO_contract* 2^>nul') do (
    set FILE_NAME=%%F
    goto :found
)

echo No NSE_FO_contract file found. Exiting...
exit /b

:found
echo %CURRENTDATE% FO Contract File: %FILE_NAME%

:: Check if the file exists
if not exist "%FILE_NAME%" (
    echo %FILE_NAME% not found. Please check the file!
    exit /b
)

set "TEMP_FILE=temp.csv"

:: Use PowerShell for better performance
powershell -Command "(Get-Content '%FILE_NAME%') | Select-Object -Skip 1 | Set-Content '%TEMP_FILE%'"

:: Check if sqlite3.exe exists
if not exist "%SQLITE_EXE%" (
    echo Error: sqlite3.exe not found. Place it in the same directory as this script.
    exit /b 1
)

:: Check if database exists, create it if not
if not exist "%DB%" (
    echo Creating database %DB%...
    "%SQLITE_EXE%" "%DB%" "VACUUM;"
    if exist "%DB%" (
        echo Database created successfully.
    ) else (
        echo Failed to create database.
        exit /b 1
    )
) else (
    echo Database %DB% already exists.
)

:: Create table if not exists
sqlite3.exe %DB% < query.sql
echo Table created and data cleared.


:: Import the CSV into SQLite
echo Importing CSV data...
echo .mode csv > sqlite_commands.sql
echo .import "%TEMP_FILE%" contract >> sqlite_commands.sql
"%SQLITE_EXE%" "%DB%" < sqlite_commands.sql

:: Cleanup
del sqlite_commands.sql
del "%TEMP_FILE%"

echo CSV data imported successfully.

echo Updating ResultSet ....
sqlite3.exe %DB% < InsertQuery.sql 

echo ResultSet updated for %FILE_NAME% ... DONE

echo Process Completed 




