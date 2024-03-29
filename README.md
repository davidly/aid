# aid
Aggregate Image Data. Runs reports like summarizing camera and lenses usage in a set of images.

Reports include finding all DNG/TIF/JPG files with embedded Adobe edit instructions, extracting
images from flac/mp3 files in bulk, summarizing focal lengths used for images, finding photos
with GPS data, summarizing camera models, and extracting serial numbers for lenses and camera
for images. This works for most RAW formats.

To build, use a Visual Studio 64 bit command prompt and run m.bat

Usage

    usage: aid [filename] /p:[rootpath] /e:[extesion] /a:X /m:[model] [/v]
    Aggregate Image Data
           filename       Retrieves data of just one file. Can't be used with /p and /e.
           /a:X           App Mode. Default is Serial Numbers
                              a   Adobe Edits
                              e   Embedded Images (flac/mp3)
                              f   Focal Lengths
                              g   GPS data
                              i   Valid image embedded (e.g. flac files)
                              l   Lens Models
                              m   Models
                              n   F Numbers
                              s   Serial Numbers
                              r   Rating (0-5 in XMP data)
           /c             Used with /a:e, creates a file for each embedded image in the 'out' subdirectory.
           /e:            Specifies the file extension to include. Default is *
           /m:            Used with /p and /e. The model substring must be in the EquipModel case insensitive.
           /o             Use One thread for parsing files, not parallelized. (enumeration uses many threads).
           /p:            Specifies the root of the file system enumeration.
           /s:X           Sort criteria. Default is App Mode setting /a
                              c   Count of entries
           /v             Enable verbose tracing.
       examples:    aid c:\pictures\whitney.jpg
                    aid /p:c:\pictures /e:jpg
                    aid /a:f /p:c:\pictures /e:jpg
                    aid /p:c:\pictures /e:dng /m:leica
                    aid /p:d:\ /e:cr? /a:l /s:c
                    aid /p:d:\ /e:rw2 /a:m /s:c
       notes:       Supported extensions: JPG, TIF, RW2, RAF, ARW, .ORF, .CR2, .CR3, .NEF, .DNG, .FLAC, .MP3, etc.

Sample output for finding lenses used for photos taken with Fujifilm bodies:

    C:\>aid /p:d:\zdrive\pics /e:raf /a:l
    found 23544 files

    found 21 unique lenses in 23109 files with that data
    make                           model                                            count
    ----                           -----                                            -----
    Carl Zeiss                     Touit 2.8/12                                     1768
    FUJIFILM                       XC15-45mmF3.5-5.6 OIS PZ                         101
    FUJIFILM                       XC16-50mmF3.5-5.6 OIS                            3319
    FUJIFILM                       XF100-400mmF4.5-5.6 R LM OIS WR                  158
    FUJIFILM                       XF100-400mmF4.5-5.6 R LM OIS WR + 2x             27
    FUJIFILM                       XF16-55mmF2.8 R LM WR                            2644
    FUJIFILM                       XF16-80mmF4 R OIS WR                             310
    FUJIFILM                       XF16mmF1.4 R WR                                  766
    FUJIFILM                       XF18-135mmF3.5-5.6R LM OIS WR                    2892
    FUJIFILM                       XF18-55mmF2.8-4 R LM OIS                         726
    FUJIFILM                       XF18mmF2 R                                       1033
    FUJIFILM                       XF23mmF1.4 R                                     295
    FUJIFILM                       XF23mmF2 R WR                                    1344
    FUJIFILM                       XF27mmF2.8                                       121
    FUJIFILM                       XF35mmF1.4 R                                     2660
    FUJIFILM                       XF35mmF2 R WR                                    1607
    FUJIFILM                       XF50mmF2 R WR                                    136
    FUJIFILM                       XF55-200mmF3.5-4.8 R LM OIS                      575
    FUJIFILM                       XF56mmF1.2 R                                     1514
    FUJIFILM                       XF60mmF2.4 R Macro                               1070
    FUJIFILM                       XF8-16mmF2.8 R LM WR                             43

Sample output for finding focal lengths used for photos taken with Leica bodies:

    C:\>aid /p:d:\zdrive\pics /e:dng /a:f /m:leica
    found 6332 files

    found 5 unique focal lengths in 5238 files with that data
    focal length        count
    ------------        -----
              24          256
              28         3641
              35          257
              50         1074
              75           10
              
Sample output for finding focal lengths used for photos taken with a Canon G7-X Mark II:

    C:\>aid /p:d:\zdrive\pics /e:cr2 /a:f "/m:G7 X Mark II"
    found 61053 files

    found 63 unique focal lengths in 13963 files with that data
    focal length        count
    ------------        -----
              24         6296
              26           10
              27           33
              28           63
              29           39
              30           52
              31           70
              32           97
              33           90
              34           82
              35           87
              36           94
              37          120
              38          188
              39           95
              40          181
              41          147
              42          119
              43          160
              44          166
              45          114
              46          156
              47           86
              48           67
              49           53
              50          154
              51          229
              52           44
              53           70
              54          206
              55           52
              56           65
              57          192
              58           67
              59           59
              60           87
              61           79
              62           75
              63           46
              64          122
              65           53
              66           31
              67          110
              68           63
              69           77
              70           62
              71          100
              72           56
              73           86
              75           46
              76           30
              77           50
              79           76
              80           24
              82           41
              83           54
              85           54
              86           29
              88           72
              90           56
              92           35
              95           68
             100         2478
             
         
