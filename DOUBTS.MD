Discovery → Poole 

I cannot find where to:
Frame KO connection. It is sent if the connection could not be established. 

----------------------------------------------------------------

Discovery.c:  WHY?

            |___________FRAME_____________|   
            | TYPE: 1
            | HEADERLEnght: 9
            | HEADER: NEW_POOLE
         →  &85330.0.1nke

----------------------------------------------------------------         

crtf + shift + f = else if(strcmp(frame.header, "EXIT") == 0)

I changed the body so I can differ from poole or bowman