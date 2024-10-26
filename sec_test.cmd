/*********************************************************************\
**  SEC_TEST.CMD :                                                   **
**        (C) 1994,1997 Thomas Christinck, Braunschweig, Germany     **
**                      Email T.Christinck@Teamos2.de                **
***********************************************************************
**  This REXX-script is just testing the SECTOR.DLL                  **
**********************************************************************/

SIGNAL ON HALT NAME END

call rxfuncadd sysloadfuncs,rexxutil,sysloadfuncs
call sysloadfuncs

call RxFuncAdd 'QDrive',   'sector', 'QDrive'
call RxFuncAdd 'QPDisk',   'sector', 'QPDisk'
call RxFuncAdd 'ReadSect', 'sector', 'ReadSect'
call RxFuncAdd 'WritSect', 'sector', 'WritSect'
call RxFuncAdd 'ReadPSect','sector', 'ReadPSect'


/**********************************************************************/

driveinfo:

say "Info all local drives :"

disks= sysdrivemap(,'local')
say "drive sectors cylinders heads sec/track byte/sec       MB"
do i= 1 to words(disks)
  lw=word(disks,i)
  info=QDrive(lw)
  if left(info,5)=="Error" then say lw  info
  else do
     say  lw right(word(info,1),10)  right(word(info,2),9) right(word(info,3),5) ,
             right(word(info,4),9) right(word(info,5),8) format(word(info,1)*word(info,5)/1024/1024,6,1)
     end
  end

"pause"

/**********************************************************************/

readsector:

say "Reading first sector of drive C:"

sect = readsect("C:", 0)

if left(sect,5)=="Error" then do
    say sect
    signal ende
    end

do i=0 to 511 by 16
     call charout , d2x(i,4)"  "
     do j=1 to 16
        call charout , c2x(substr(sect,i+j,1))" "
        end
     do j=1 to 16
        c = substr(sect,i+j,1)
        if c<" " then call charout ," "
                 else call charout ,c
        end
     say
     end

"pause"

/********************************************************************/


say "Info about first physical partitionable drive"

info = QPDisk("1:")
if left(info,5)=="Error"
   then say  info
   else say  "Cylinders :" word(info,1) "Heads :" word(info,2) "Sectors :" word(info,3)


"pause"

/********************************************************************/

say "Read first sector of first physical drive :"

sect = ReadPSect("1:", 0, 0, 0)

if left(sect,5)=="Error" then do
    say sect
    signal ende
    end

do i=0 to 511 by 16
     call charout , d2x(i,4)"  "
     do j=1 to 16
        call charout , c2x(substr(sect,i+j,1))" "
        end
     do j=1 to 16
        c = substr(sect,i+j,1)
        if c<" " then call charout ," "
                 else call charout ,c
        end
     say
     end

signal ende

/********************************************************************/

End:
say "^C"
say " Break ...."

ende:
call RxFuncDrop 'QDrive'
call RxFuncDrop 'ReadSect'
call RxFuncDrop 'WritSect'
call RxFuncDrop 'ReadPSect'

