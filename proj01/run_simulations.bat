@echo off
for %%t in (1 2 4 8) do (
    for %%n in (1000 10000 100000) do (
        msbuild /p:Configuration=Release /p:DefineConstants="NUMT=%%t;NUMTRIALS=%%n" proj01.sln
        proj01.exe
    )
)