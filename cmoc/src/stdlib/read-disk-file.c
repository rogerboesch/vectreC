/*  read-disk-file.c - DECB-type file reading functions.

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#include "cmoc-stdlib-private.h"


// filename: 11-byte DECB directory entry (with filename padded to 8 with spaces).
//           Example: "FOO     TXT"
// Returns the offset of the 16-byte directory entry in dirSector[] (i.e., 0, 32, ..., 224),
//         or 256 if a read error occurred;
//         or 257 if no matching entry was found.
//
size_t
findDirEntry(byte dirSector[256], byte driveNo, const char filename[11],
             _CMOC_ReadDiskSectorFuncPtr readDiskSectorFuncPtr)
{
    for (byte sector = 3; sector <= 18; ++sector)
    {
        if (!(*readDiskSectorFuncPtr)(dirSector, driveNo, 17, sector))
            return 256;  // read error

        byte *entry;
        for (word index = 0; index < 256; index += 32)
        {
            entry = dirSector + index;
            if (!*entry)  // if erased entry
                continue;
            if (*entry == 0xFF)  // if end of dir
                break;

            if (memcmp(entry, filename, 11) == 0)  // if filename matches
                return entry - dirSector;  // found
        }
        if (*entry == 0xFF)  // if end of dir
            break;
    }

    return 257;  // not found
}


// filename: 11-byte DECB directory entry (with filename padded to 8 with spaces).
//           Example: "FOO     TXT"
// numBytesInLastSector: Non-null pointer to a word that receives the number of
//                       valid bytes (1..256) in the file's last sector.
// Returns:
//   0 = success;
//   1 = read error when reading directory sector(s);
//   2 = file not found;
//   3 = read error when reading the FAT.
//
// Upon success, leaves the FAT at the start of sectorBuffer[].
// *fd does not keep a copy of the sectorBuffer address.
//
static byte
openDECBFile(struct _CMOC_FileDesc *fd, byte driveNo, const char filename[11],
             byte sectorBuffer[256], unsigned short *numBytesInLastSector,
             _CMOC_ReadDiskSectorFuncPtr readDiskSectorFuncPtr)
{
    const size_t entryOffsetInSector = findDirEntry(sectorBuffer,
                                                    driveNo, filename,
                                                    readDiskSectorFuncPtr);
    if (entryOffsetInSector >= 256)
        return (byte) (entryOffsetInSector - 255);  // read error or file not found

    fd->driveNo = driveNo;
    fd->curGran = (sectorBuffer + entryOffsetInSector)[13];
    fd->curSec = 1;
    fd->readDiskSectorFuncPtr = readDiskSectorFuncPtr;
    *numBytesInLastSector = * (unsigned short *) (sectorBuffer + entryOffsetInSector + 14);

    // Read FAT sector.
    //
    if (!(*fd->readDiskSectorFuncPtr)(sectorBuffer, driveNo, 17, 2))
        return 3;  // read error

    // Check for empty file, to avoid reading a data sector for nothing.
    //
    if (sectorBuffer[fd->curGran] == 0xC1 && *numBytesInLastSector == 0)
        fd->curGran = 0xFF;  // indicate to getNextDECBSector() that EOF reached

    return 0;
}


// Output:
// *track: 0..16, 18..34.
// *sec: 0 or 9.
//
static void
granuleToTrack(byte granule, byte *track, byte *sec)
{
    *track = granule >> 1;
    if (*track >= 17)  // if granule is after dir track
        ++*track;
    *sec = ((granule & 1) > 0 ? 9 : 0);  // 0 for even numbered granule, 9 for odd
}


// Returns:
//   0 = sector successfully read and stored in sectorBuffer[];
//   1 = no more sectors to read (not a failure);
//   2 = read error.
//
static byte
getNextDECBSector(struct _CMOC_FileDesc *fd,
                  byte sectorBuffer[256],
                  const byte fatBuffer[MAX_NUM_GRANULES])
{
    if (fd->curGran == 0xFF)  // if at EOF
        return 1;

    byte track;
    byte sec;
    granuleToTrack(fd->curGran, &track, &sec);
    if (!(*fd->readDiskSectorFuncPtr)(sectorBuffer, fd->driveNo, track, sec + fd->curSec))
        return 2;

    // Sector read successfully.

    // Determine the number of sectors in the current granule that are
    // part of the file (1..9).
    //
    byte g = fatBuffer[fd->curGran];
    byte numSectorsCurGran;
    if (g >= 0xC1)
        numSectorsCurGran = g - 0xC0;
    else
        numSectorsCurGran = 9;

    // Advance sector index. Go to next granule if needed.
    //
    ++fd->curSec;
    if (fd->curSec > numSectorsCurGran)  // if current granule finished
    {
        if (g >= 0xC1)  // if current granule is last
            fd->curGran = 0xFF;  // marks file descriptor as at EOF, for next call
        else
        {
            fd->curSec = 1;
            fd->curGran = g;
        }
    }
    return 0;
}


byte
_CMOC_readDECBFile2(void *dest,
                    byte driveNo, const char filename[11],
                    byte workBuffer[256], size_t *sizePtr,
                    _CMOC_ReadDiskSectorFuncPtr readDiskSectorFuncPtr)
{
    void *writer = dest;
    struct _CMOC_FileDesc fd;
    unsigned short numBytesInLastSector;
    byte err = openDECBFile(&fd, driveNo, filename,
                            workBuffer, &numBytesInLastSector, readDiskSectorFuncPtr);  // returns 0..3
    if (err == 0)
        for (;;)
        {
            err = getNextDECBSector(&fd, (byte *) writer, workBuffer);  // returns 0..2
            if (err == 0)
            {
                writer += 256;
                continue;
            }
            if (err == 1)
            {
                err = 0;  // EOF, so success
                break;
            }
            if (err != 0)
            {
                err += 2;  // now 4 (read error)
                break;
            }
        }

    disableInterrupts();

    // N.B.: Interrupts are deliberately left masked.
    //       The caller must call enableInterrupts() when interrupts are wanted again.

    if (err == 0)
    {
        if (writer > dest)  // if at least one sector read:
            *sizePtr = writer - dest - 256 + numBytesInLastSector;
        else
            *sizePtr = 0;
    }

    return err;
}
