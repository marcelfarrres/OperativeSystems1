- INSTRUCTIONS

    1) Here we have a structure of a server with all folders and names. 
    I assumed that any songs that are placed in a playlist will be placed also in the server itself,
    so if you place a new song in any playlist you also need to place it in the general server folder,
    otherwise, when you try to download the playlist it will crash, as the code downloads the songs from the 
    general folder. This helps to save memory, as you can place a empty text file in a playlist named "Song.mp3" 
    and have the origianl in the general folder. ()

    However, if you place a song in the general folder but not in any playlist, the program functions correctly.

        /serverWest
        ├── /playlists
        │   ├── /kabanaMusic
        │   │   └── Beautiful Day.mp3 
        │   │   └── Wonderwall - Remastered.mp3  <------ This file is an empty text file with the name of the song.  
        │   ├── /mamoKingz
        │   │   ├── Linger.mp3   <----------
        │   │   ├── Luka.mp3
        │   │   └── Mad World.mp3
        │   └── /Regaeton56
        │       ├── Beautiful Day.mp3
        │       ├── Give Me One Reason.mp3
        │       └── Wonderwall - Remastered.mp3
        |
        |
        ├── Beautiful Day.mp3
        ├── Give Me One Reason.mp3
        ├── Linger.mp3   <----------
        ├── Luka.mp3
        ├── Mad World.mp3
        └── Wonderwall - Remastered.mp3

    2) We added a Command Shortcut, so can use the normal command or the number, EXAMPLE: [$ 1] = [$ CONNECT]
        (to download a playlist you need to put two sixes before the playlist name)

        ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
        ┃ Commands Shortcuts:
        ┃
        ┃  1) CONNECT            5) DOWNLOAD
        ┃  2) LOGOUT             6 6) DOWNLOAD PLAYLIST (double 6)
        ┃  3) LIST SONGS         7) CHECK DOWNLOADS
        ┃  4) LIST PLAYLISTS     8) CLEAR DOWNLOADS
        ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛


   3) MAKEFILE

    We have 2 types of commands: 

        - To run WITHOUT valgrind:

            Discovery: make d
            Poole: make p1, make p2
            Bowman: make b1, make b2, make b3

        - To run WITH valgrind:

            Discovery: make vd
            Poole: make vp1, make vp2, make vp3
            Bowman: make vb1, make vb2, make vb3, make vb4

    We recommend you to use the VALGRIND as it shows all the memory being freed in the end.




