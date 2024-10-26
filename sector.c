/*-------------------------------------------------------------------*/
/* SECTOR.C read/write of Sectors on Floppys and HD's                */
/*-------------------------------------------------------------------*/

#define INCL_BASE
#define INCL_DOSDEVIOCTL

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_32
#define INCL_REXXSAA
#include <rexxsaa.h>

#define BUILDRXSTRING(t, s) {strcpy((t)->strptr,(s)); (t)->strlength=strlen((s)); }

RexxFunctionHandler QDrive;
RexxFunctionHandler QPDisk;
RexxFunctionHandler ReadSect;
RexxFunctionHandler WritSect;
RexxFunctionHandler ReadPSect;

typedef _Packed struct
    {
    USHORT bytesSector;
    UCHAR  sectorsCluster;
    USHORT reservedSectors;
    UCHAR  numFats;
    USHORT rootDirEntries;
    USHORT totallSectors;
    UCHAR  mediaDescriptor;
    USHORT sectorsFat;
    USHORT sectorsTrack;
    USHORT numHeads;
    ULONG  hiddenSectors;
    ULONG  largeTotalSectors;
    CHAR   reserved[6];
    USHORT numCylinders;
    UCHAR  type;
    USHORT attributes;
    } diskLDevice;

typedef _Packed struct
    {
    USHORT reserved1;
    USHORT numCylinders;
    USHORT numHeads;
    USHORT sectorsTrack;
    USHORT reserved2;
    USHORT reserved3;
    USHORT reserved4;
    USHORT reserved5;
    } diskPDevice;

typedef struct
    {
    UCHAR command;
    UCHAR drive;
    } parmList;

void error(char *msg, int error);

/*-------------------------------------------------------------------*/
/* QDRIVE  - Query number of sectors                                 */
/*-------------------------------------------------------------------*/

ULONG QDrive(PUCHAR Name, ULONG argc, RXSTRING argv[], PSZ Queuename, PRXSTRING Retstr)
  {
    char ret[100];
    char *szDrive;
    HFILE fhFile;
    diskLDevice device;
    ULONG deviceLength = sizeof(device);
    parmList parm;
    ULONG parmLength   = sizeof(parm);
    ULONG ulAction;
    APIRET rc;

    szDrive = argv[0].strptr;

    rc = DosOpen(szDrive, &fhFile, &ulAction, 0L, FILE_NORMAL,
        OPEN_ACTION_OPEN_IF_EXISTS,
        OPEN_FLAGS_DASD             |
        OPEN_FLAGS_WRITE_THROUGH    |
        OPEN_FLAGS_NOINHERIT        |
        OPEN_SHARE_DENYREADWRITE    |
        OPEN_ACCESS_READONLY,    0L);
    if (rc) {
         sprintf(ret, "Error: %d (DosOpen)", rc);
         BUILDRXSTRING(Retstr, ret);
         return(0);
         }

    parm.command = 1;

    rc = DosDevIOCtl(fhFile, 0x08L, 0x63L,
                     &parm, parmLength, &parmLength,
                     &device, deviceLength, &deviceLength);
    if (rc) {
         sprintf(ret, "Error: %d (DosDevIOCtl,8,63)", rc);
         BUILDRXSTRING(Retstr, ret);
         return(0);
         }

    rc = DosClose(fhFile);
    if (rc) {
         sprintf(ret, "Error: %d (DosClose)", rc);
         BUILDRXSTRING(Retstr, ret);
         return(0);
         }

    rc = max(device.largeTotalSectors, device.totallSectors);

    sprintf(ret, "%d %d %d %d %d", rc, device.numCylinders, device.numHeads,
                             device.sectorsTrack, device.bytesSector);
    BUILDRXSTRING(Retstr, ret);
    return(0);
    }


ULONG ReadSect(PUCHAR Name, ULONG argc, RXSTRING argv[], PSZ Queuename, PRXSTRING Retstr)
    {
    int sec_no;
    char *szDrive;
    HFILE fhFile;
    ULONG BytesRead, ulAction, NewPtr;
    char buffer[515], ret[100];
    APIRET rc;

    if (argc != 2)
        return 40;

    szDrive = argv[0].strptr;
    sec_no = atoi(argv[1].strptr);

    rc = DosOpen(szDrive, &fhFile, &ulAction, 0L, FILE_NORMAL,
        OPEN_ACTION_OPEN_IF_EXISTS,
        OPEN_FLAGS_DASD             |
        OPEN_FLAGS_WRITE_THROUGH    |
        OPEN_FLAGS_NOINHERIT        |
        OPEN_SHARE_DENYREADWRITE    |
        OPEN_ACCESS_READONLY , 0L);
    if (rc){
        sprintf(ret, "Error: %d (DosOpen)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosSetFilePtr(fhFile, sec_no*512,0, &NewPtr);
    if (rc){
        DosClose(fhFile);
        sprintf(ret, "Error: %d (DosSetFilePtr)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosRead(fhFile, buffer, 512, &BytesRead);
    if (rc){
        DosClose(fhFile);
        sprintf(ret, "Error: %d (DosRead)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosClose(fhFile);

    Retstr->strptr=buffer;
    Retstr->strlength=512;
    return 0;
    }


ULONG WritSect(PUCHAR Name, ULONG argc, RXSTRING argv[], PSZ Queuename, PRXSTRING Retstr)
    {
    char *szDrive, *ptr;
    UCHAR ret[100];
    HFILE fhFile;
    ULONG sec_no, BytesRead, ulAction, NewPtr;
    APIRET rc;

    if (argc != 3)
        return 40;

    szDrive = argv[0].strptr;
    sec_no = atoi(argv[1].strptr);
    ptr = argv[2].strptr;

    rc = DosOpen(szDrive, &fhFile, &ulAction, 0L, FILE_NORMAL,
        OPEN_ACTION_OPEN_IF_EXISTS,
        OPEN_FLAGS_DASD             |
        OPEN_FLAGS_WRITE_THROUGH    |
        OPEN_FLAGS_NOINHERIT        |
        OPEN_SHARE_DENYREADWRITE    |
        OPEN_ACCESS_READWRITE, 0L);
    if (rc){
        sprintf(ret, "Error: %d (DosOpen)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosDevIOCtl(fhFile, 0x08L, 0x00L, 0, 0, 0, 0, 0, 0);
    if (rc){
        DosClose(fhFile);
        sprintf(ret, "Error: %d (DosDevIOCtl 8,0)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosSetFilePtr(fhFile, sec_no*512,0, &NewPtr);
    if (rc){
        DosDevIOCtl(fhFile, 0x08L, 0x01L, 0, 0, 0, 0, 0, 0);
        DosClose(fhFile);
        sprintf(ret, "Error: %d (DosSetFilePtr)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosWrite(fhFile, ptr, 512, &BytesRead);
    if (rc){
        DosDevIOCtl(fhFile, 0x08L, 0x01L, 0, 0, 0, 0, 0, 0);
        DosClose(fhFile);
        sprintf(ret, "Error: %d (DosWrite)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosDevIOCtl(fhFile, 0x08L, 0x01L, 0, 0, 0, 0, 0, 0);
    rc = DosClose(fhFile);

    Retstr->strlength=1;
    return 0;
    }

ULONG QPDisk(PUCHAR Name, ULONG argc, RXSTRING argv[], PSZ Queuename, PRXSTRING Retstr)
  {
    char ret[100];
    char *szDisk;
    HFILE fhFile;
    diskPDevice device;
    ULONG deviceLength = sizeof(device);
    parmList parm;
    ULONG parmLength   = sizeof(parm);
    ULONG ulAction;
    APIRET rc;

    szDisk = argv[0].strptr;

    rc = DosPhysicalDisk(2, &fhFile, 2, szDisk ,3);
    if (rc){
        sprintf(ret, "Error: %d (DosPhysicalDisk,2)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    parm.command = 1;

    rc = DosDevIOCtl(fhFile, 0x09L, 0x63L,
                     &parm, parmLength, &parmLength,
                     &device, deviceLength, &deviceLength);
    if (rc){
        sprintf(ret, "Error: %d (DosDecIOCtl,9,63)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosPhysicalDisk(3, NULL, 0, (PVOID) &fhFile , 2);
    if (rc){
        sprintf(ret, "Error: %d (DosPhysicalDisk,3)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    sprintf(ret, "%d %d %d", device.numCylinders, device.numHeads,
                             device.sectorsTrack);
    BUILDRXSTRING(Retstr, ret);
    return(0);
    }



ULONG ReadPSect(PUCHAR Name, ULONG argc, RXSTRING argv[], PSZ Queuename, PRXSTRING Retstr)

    {
    HFILE fhFile;
    UCHAR TrackBuf[sizeof(TRACKLAYOUT)+256*2*sizeof(USHORT)];
    TRACKLAYOUT *Track = (PVOID) TrackBuf;
    ULONG TrackLength=sizeof(TrackBuf);
    USHORT usHead, usCyl, usSect, i;
    UCHAR data[2000], ret[100], *szDisk;
    ULONG dataLength=sizeof(data);
    APIRET rc;

    if (argc != 4)
        return 40 ;

    szDisk = argv[0].strptr;
    usCyl  = atoi(argv[1].strptr);
    usHead = atoi(argv[2].strptr);
    usSect = atoi(argv[3].strptr);


    rc = DosPhysicalDisk(2, &fhFile, 2, szDisk ,3);
    if (rc){
        sprintf(ret, "Error: %d (DosPhysicalDisk,2)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    Track->bCommand = 1;
    Track->usHead = usHead;
    Track->usCylinder = usCyl;
    Track->usFirstSector = usSect;
    Track->cSectors = 1;
    for(i=0;i<256;i++){
       Track->TrackTable[i].usSectorNumber=i+1;
       Track->TrackTable[i].usSectorSize=512;
       }

    rc = DosDevIOCtl(fhFile, 0x09L, 0x64L,
                     Track, TrackLength, &TrackLength,
                     &data, dataLength, &dataLength);
    if (rc){
        sprintf(ret, "Error: %d (DosDevIOCtl,9,64)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    rc = DosPhysicalDisk(3, NULL, 0, (PVOID) &fhFile , 2);
    if (rc){
        sprintf(ret, "Error: %d (DosPhysicalDisk,3)", rc);
        Retstr->strptr=ret;
        Retstr->strlength=strlen(ret);
        return 0;
        }

    Retstr->strptr=data;
    Retstr->strlength=512;
    return 0;
    }
