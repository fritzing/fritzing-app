cd "C:\fritzing2\phoenix\part-gen-scripts\cheetah scripts"
C:\Python26\python.exe partomatic.py -c .\configs\sip_cfg.cfg -t .\templates\dip.cfg -o .\deleteme\config
C:\Python26\python.exe partomatic.py -c .\deleteme\config\sip_300mil_fzp.cfg -t .\templates\sip.fzp -o .\deleteme\fzp
C:\Python26\python.exe partomatic.py -c .\deleteme\config\sip_300mil_bread_svg.cfg -t .\templates\sip_bread.svg -o .\deleteme\bread
C:\Python26\python.exe partomatic.py -c .\deleteme\config\sip_300mil_schem_svg.cfg -t .\templates\sip_schem.svg -o .\deleteme\schem

