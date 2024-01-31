#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>

#include <devices/trackdisk.h>
#include <proto/exec.h>

#include <dbg/dbg.h>


#define VERSTAG "\0$VER: read_track 1.0 (31/01/2024) Brewed for you by Pepito\0"


#define LEN_FILENAME 128

typedef struct _TrackContext {
    int count;
    int floppy_unit;
    int first;
    FILE *out;
    struct IOExtTD *request;
    int sector_size;
    int sectors_per_track;
    char file_name[LEN_FILENAME];
    bool sector_mode;
    bool verbose;
    bool opened;
} TrackContext;


static bool isprint(char c)
{
    return ((c >= ' ') && (c <= '~'));
}

#define BYTES_PER_LINE 16

static void HexDump(void *buffer, int size)
{
    char *ptr = (char *)buffer;
    char ascii[BYTES_PER_LINE + 1];
    int idx = 0;
    int ofs = 0;

    check(ASSIGNED(buffer), "Null buffer");

    ascii[BYTES_PER_LINE] = '\0';
    printf("%04x  ", ofs);
    while(size > 0)
    {
        printf("%02hhx ", *ptr);
        if(isprint(*ptr))
        {
            ascii[idx] = *ptr;
        }
        else
        {
            ascii[idx] = '.';
        }

        idx++;
        ptr++;
        size--;

        if(idx == BYTES_PER_LINE)
        {
            printf(" %s\n", ascii);
            if(size != 0)
            {
                ofs += BYTES_PER_LINE;
                printf("%04x: ", ofs);
            }
            idx = 0;
        }
    }
    if(idx != 0)
    {
        ascii[idx] = '\0';
        while(idx < BYTES_PER_LINE)
        {
            printf("   ");
            idx++;
        }
        printf(" %s\n", ascii);
    }

on_error:
    return;
}


static void StopDriveMotor(struct IOExtTD *io)
{
    io->iotd_Req.io_Command = TD_MOTOR;
    io->iotd_Req.io_Length = 0;
    check(0 == DoIO((struct IORequest *)io), "Unable to stop the drive motor");

on_error:
    return;
}


void tc_CleanUp(TrackContext *tc)
{
    struct MsgPort *port;

    if(ASSIGNED(tc))
    {
        if(ASSIGNED(tc->request))
        {
            port = tc->request->iotd_Req.io_Message.mn_ReplyPort;
            if(tc->opened)
            {
                StopDriveMotor(tc->request);
            }
            DeleteIORequest((struct IORequest *)tc->request);
            tc->request = NULL;
            if(ASSIGNED(port))
            {
                DeleteMsgPort(port);
            }
        }
        if(NULL != tc->out)
        {
            fclose(tc->out);
            tc->out = NULL;
        }
    }
}


static void Usage(char *progname)
{
    fprintf(stderr, "USAGE: %s {-t <track> | -s <sector>} [-c <count>] [-o <outfile>] [-d <unit>] [-v]\n", progname);
    fprintf(stderr, "\t-c <count>, count of tracks or sectors to read, default to 1\n");
    fprintf(stderr, "\t-d <unit>, floppy disk unit, defaults to 0 (df0:)\n");
    fprintf(stderr, "\t-o <outfile>, file to write the track(s) to, as binary,\n");
    fprintf(stderr, "\t              default is to output as hexdump\n");
    fprintf(stderr, "\t-s <sector>, number of first sector to read, 0..79\n");
    fprintf(stderr, "\t-t <track>, number of first track to read, 0..79\n");
    fprintf(stderr, "\t-v, displays more info, like drive geometry\n");
}


static bool GetArgs(int argc, char *argv[], TrackContext *tc)
{
    int opt;
    tc->count = 1;
    tc->floppy_unit = 0;
    tc->first = -1;
    tc->out = NULL;
    tc->request = NULL;
    tc->file_name[0] = '\0';
    tc->verbose = false;
    tc->opened = false;

    while((opt = getopt(argc, argv, "t:s:c:o:d:v")) != -1)
    {
        switch(opt)
        {
            case 't':
                tc->first = atoi(optarg);
                check((tc->first >= 0), "Bad track number: %d", tc->first);
                tc->sector_mode = false;
                break;

            case 's':
                tc->first = atoi(optarg);
                check((tc->first >= 0), "Bad sector number: %d", tc->first);
                tc->sector_mode = true;
                break;

            case 'c':
                tc->count = atoi(optarg);
                check(tc->count > 0, "Bad count: %d", tc->count);
                break;

            case 'd':
                tc->floppy_unit = atoi(optarg);
                check((tc->floppy_unit >= 0) && (tc->floppy_unit <= NUMUNITS), "Bad floppy unit: %d", tc->floppy_unit);
                break;

            case 'o':
                strncpy(tc->file_name, optarg, sizeof(tc->file_name) - 1);
                tc->file_name[sizeof(tc->file_name) - 1] = '\0';
                break;

            case 'v':
                tc->verbose = true;
                break;

            case '?':
            case 'h':
            case ':':
            default:
                check(false, "Help needed...");
        }
    }
    check(tc->first >= 0, "You must supply a sector or track number");

    return true;

on_error:
    Usage(argv[0]);
    return false;
}


static bool DiskInDrive(struct IOExtTD *io)
{
    io->iotd_Req.io_Command = TD_CHANGESTATE;
    check(0 == DoIO((struct IORequest *)io), "Unable to get disk presence");
    return (io->iotd_Req.io_Actual == 0);

on_error:
    return false;
}


static bool CheckArgs(TrackContext *tc)
{
    struct MsgPort *port = NULL;
    struct DriveGeometry geometry;
    int tracks;

    // if filename, check file
    if(tc->file_name[0] != '\0')
    {
        tc->out = fopen(tc->file_name, "wb");
        check(ASSIGNED(tc->out), "Unable to create file %s", tc->file_name);
    }

    // check device
    port = CreateMsgPort();
    check(ASSIGNED(port), "Unable to get a message port");

    tc->request = (struct IOExtTD *)CreateIORequest(port, sizeof(struct IOExtTD));
    check(ASSIGNED(tc->request), "Unable to get IO request");

    check(0 == OpenDevice(TD_NAME, tc->floppy_unit, (struct IORequest *)tc->request, 0L), "Unable to open DF%c:", tc->floppy_unit + '0');
    tc->opened = true;

    // check geometry
    tc->request->iotd_Req.io_Data = &geometry;
    tc->request->iotd_Req.io_Command = TD_GETGEOMETRY;
    check(0 == DoIO((struct IORequest *)tc->request), "Unable to get drive geometry");

    if(tc->verbose)
    {
        printf("DF%c: geometry\n==============\n", '0' + tc->floppy_unit);
        printf("  Sector size:      %d\n", geometry.dg_SectorSize);
        printf("  Total sectors:    %d\n", geometry.dg_TotalSectors);
        printf("  Cylinders:        %d\n", geometry.dg_Cylinders);
        printf("  Sectors/cylinder: %d\n", geometry.dg_CylSectors);
        printf("  Heads:            %d\n", geometry.dg_Heads);
        printf("  Sectors/track:    %d\n", geometry.dg_TrackSectors);
    }
    tc->sector_size = geometry.dg_SectorSize;
    tc->sectors_per_track = geometry.dg_TrackSectors;

    if(tc->sector_mode)
    {
        check(geometry.dg_TotalSectors > (tc->first + tc->count - 1),
              "Impossible to read sectors %d..%d on a %d sectors drive (0..%d)",
              tc->first, tc->first + tc->count - 1, geometry.dg_TotalSectors, geometry.dg_TotalSectors - 1);
    }
    else
    {
        tracks = geometry.dg_Cylinders * geometry.dg_Heads;
        check(tracks > (tc->first + tc->count - 1),
              "Impossible to read tracks %d..%d on a %d tracks drive (0..%d)",
              tc->first, tc->first + tc->count - 1, tracks, tracks - 1);
    }

    // check if disk present
    check(DiskInDrive(tc->request), "No disk present in DF%c:", '0' + tc->floppy_unit);

    return true;

on_error:
    return false;
}


static bool ReadSectors(struct IOExtTD *req, int size, int offset, int count, FILE *out)
{
    bool res = false;
    int i;
    APTR buffer = (APTR)AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
    check_mem(buffer);

    for(i = 0; i < count; i++, offset += size)
    {
        req->iotd_Req.io_Length = size;
        req->iotd_Req.io_Data = buffer;
        req->iotd_Req.io_Offset = offset;
        req->iotd_Req.io_Command = CMD_READ;
        check(0 == DoIO((struct IORequest *)req), "Error while reading sector %d", offset / size);

        if(ASSIGNED(out))
        {
            check(1 == fwrite((void *)buffer, size, 1, out), "Error while writing to file");
        }
        else
        {
            HexDump((void *)buffer, size);
        }
    }
    res = true;

on_error:
    if(ASSIGNED(buffer))
    {
        FreeVec(buffer);
    }
    return res;
}


static bool ReadData(TrackContext *tc)
{
    int offset;
    int count;
    int track_size;

    // on met tout en secteurs
    if(tc->sector_mode)
    {
        offset = tc->sector_size * tc->first;
        count = tc->count;
    }
    else
    {
        offset = track_size * tc->first;
        count = tc->count * tc->sectors_per_track;
    }

    return ReadSectors(tc->request, tc->sector_size,  offset, count, tc->out);
}


int main(int argc, char *argv[])
{
    UBYTE versiontag[] = VERSTAG;

    int res = EXIT_FAILURE;
    TrackContext tc;

    check(true == GetArgs(argc, argv, &tc), "Aborted.");
    check(true == CheckArgs(&tc), "Aborted.");

    // lecture
    check(ReadData(&tc), "Aborted.");

    res = EXIT_SUCCESS;

on_error:
    tc_CleanUp(&tc);
    return res;
}

