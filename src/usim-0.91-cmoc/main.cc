//
//	main.cc
//

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "mc6809.h"

#if defined(__unix) || (defined(__APPLE__) && defined(TARGET_OS_MAC)) || defined(__MINGW32__)
#include <unistd.h>
#endif

#if defined(__CYGWIN__)
#include <sys/select.h>
#endif

#include <iostream>
#include <iomanip>

using namespace std;

#ifdef __osf__
extern "C" unsigned int alarm(unsigned int);
#endif

//#ifndef sun
//typedef void SIG_FUNC_TYP(int);
//typedef SIG_FUNC_TYPE *SIG_FP;
//#endif

class sys : virtual public mc6809 {

};



class Console : virtual public mc6809 {
public:
	Console() : delayTicks(0), binaryMode(false) {}
	void setBinaryMode(bool m) { binaryMode = m; }

protected:

	virtual Byte			 read(Word);
	virtual void			 write(Word, Byte);

private:

	Word	delayTicks;
	bool    binaryMode;  // if true, CRs are not translated to LFs on output to $FF00

};


// Returns 0 if no char is ready from stdin.
//
static Byte
getcharifavail()
{
	#if defined(__MINGW32__)
	return 0;  // not implemented: need to #include the right header file to define select(2)
	#else
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(STDIN_FILENO, &readset);
	if (select(STDIN_FILENO + 1, &readset, NULL, NULL, NULL) <= 0)
		return 0;
	if (!FD_ISSET(STDIN_FILENO, &readset))
		return 0;
	char c;
	if (read(STDIN_FILENO, &c, 1) != 1)
		return 0;
	return Byte(c);
	#endif
}


class DSKCON
{
public:
    DSKCON()
    :   DCOPC(255), DCDRV(0), DCTRK(0), DCSEC(1), DCBPT(0), DCSTA(0), dskFile(fopen("usim.dsk", "r+b")), usim(NULL), verbose(true)
    {
        //if (dskFile == NULL && verbose)
        //    cerr << "usim: failed to open usim.dsk in current directory" << endl;
    }

    // Must be called before calling read() or write().
    // _usim: Must not be null.
    //
    void setUSim(USim *_usim)
    {
        usim = _usim;
    }

    ~DSKCON()
    {
        if (dskFile != NULL)
            fclose(dskFile);
    }

    Byte read(Byte addr);
    void write(Byte addr, Byte b);

private:
    void execute();
    bool trySeek(long &byteOffset) const;
    bool areVariablesValid() const
    {
        return dskFile != NULL
            && (DCOPC == 2 || DCOPC == 3)  // only sector read and write operations supported
            && DCDRV == 0  // only drive 0 supported
            && DCTRK < 35
            && DCSEC >= 1
            && DCSEC <= 18
            && DCBPT <= 0xFD00  // protect $FExx (mini-OS) and $FFxx (I/O ports)
            ;
    }

private:
    // Forbidden:
    DSKCON(const DSKCON &);
    DSKCON &operator = (const DSKCON &);

private:
    Byte DCOPC;  // operation code
    Byte DCDRV;  // 0..3
    Byte DCTRK;  // 0..34
    Byte DCSEC;  // 1..18
    Word DCBPT;  // buffer pointer
    Byte DCSTA;  // status
    FILE *dskFile;  // raw data 35-track .dsk image
    USim *usim;  // simulator whose memory we read or write sectors from or to
    bool verbose;
};


static DSKCON dskcon;


// addr: 0..6 (DCOPC to DCSTA)
//
Byte
DSKCON::read(Byte addr)
{
    switch (addr)
    {
    case 0:
        return DCOPC;
    case 1:
        return DCDRV;
    case 2:
        return DCTRK;
    case 3:
        return DCSEC;
    case 4:
        return Byte(DCBPT >> 8);  // MSB
    case 5:
        return Byte(DCBPT & 0xFF);
    case 6:
        return DCSTA;
    default:
        return 0;  // fail silently
    }
}

// addr: see simulateDSKCONRead().
// b: byte to write
//
void
DSKCON::write(Byte addr, Byte b)
{
    switch (addr)
    {
    case 0:
        DCOPC = b;
        execute();
        break;
    case 1:
        DCDRV = b;
        break;
    case 2:
        DCTRK = b;
        break;
    case 3:
        DCSEC = b;
        break;
    case 4:
        DCBPT &= 0x00FF;        // clear MSB
        DCBPT |= Word(b) << 8;  // replace MSB
        break;
    case 5:
        DCBPT &= 0xFF00;  // clear LSB
        DCBPT |= b;       // replace LSB
        break;
    case 6:
        DCSTA = b;
        break;
    default:
        ;  // fail silently
    }
}


void
DSKCON::execute()
{
    DCSTA = 0x80;  // presume failure; use Not Ready as fallback error code

    Byte buffer[256];
    long byteOffset = 0;

    switch (DCOPC)
    {
    case 2:  // read sector
    {
        if (!trySeek(byteOffset))
            break;

        if (fread(buffer, 1, sizeof(buffer), dskFile) != sizeof(buffer))
        {
            if (verbose)
                cerr << "usim: failed to read from file offset " << byteOffset << " in usim.dsk" << endl;
            break;
        }

        #if 0  // debugging
        cout << "SECTOR READ:\n" << hex;
        for (size_t i = 0; i < sizeof(buffer); ++i)
        {
            cout << setw(2) << Word(buffer[i]) << " ";
            if (i % 16 == 15)
                cout << "\n";
        }
        #endif

        // Write the sector to the 6809's memory.
        //
        for (size_t i = 0; i < sizeof(buffer); ++i)
            usim->write(DCBPT + i, buffer[i]);

        break;
    }

    case 3:  // write sector
    {
        if (!trySeek(byteOffset))
            break;

        // Read the sector from the 6809's memory.
        //
        for (size_t i = 0; i < sizeof(buffer); ++i)
            buffer[i] = usim->read(DCBPT + i);

        #if 0  // debugging
        cout << "SECTOR WRITE:\n" << hex;
        for (size_t i = 0; i < sizeof(buffer); ++i)
        {
            cout << setw(2) << Word(buffer[i]) << " ";
            if (i % 16 == 15)
                cout << "\n";
        }
        #endif

        if (fwrite(buffer, 1, sizeof(buffer), dskFile) != sizeof(buffer))
        {
            if (verbose)
                cerr << "usim: failed to write at file offset " << byteOffset << " in usim.dsk" << endl;
            break;
        }

        break;
    }

    default:
        ;  // not supported
    }
}


bool
DSKCON::trySeek(long &byteOffset) const
{
    byteOffset = (DCTRK * 18L + DCSEC - 1) * 256;  // position in .dsk file of sector to read

    if (!areVariablesValid())
    {
        if (verbose)
            cerr << "usim: invalid read params: usim.dsk file "
                 << (dskFile ? "" : "NOT ") << "opened, drive "
                 << Word(DCDRV) << ", track " << Word(DCTRK) << ", sector " << Word(DCSEC)
                 << ", buffer $" << hex << DCBPT << dec << endl;
        return false;
    }

    if (fseek(dskFile, byteOffset, SEEK_SET) != 0)
    {
        if (verbose)
            cerr << "usim: failed to seek to " << byteOffset << " in usim.dsk" << endl;
        return false;
    }

    return true;
}


Byte Console::read(Word addr)
{
	switch (addr)
	{
	case 0xFF00:  // input port: returns ASCII code if char ready, 0 otherwise
		return getcharifavail();
	default:
	    if (addr >= 0xFF04 && addr <= 0xFF0A)
			return dskcon.read(addr - 0xFF04);
		return mc6809::read(addr);
	}
}

void Console::write(Word addr, Byte x)
{
	switch (addr)
	{
	case 0xFF00:
		cout << (char) (x == 13 && !binaryMode ? 10 : x) << flush;
		break;
	case 0xFF02:
		delayTicks = (Word(x) << 8);
		break;
	case 0xFF03:
		delayTicks |= x;
		usleep(1000000 / 60 * delayTicks);
		break;
	default:
	if (addr >= 0xFF04 && addr <= 0xFF0A)
	{
	    dskcon.setUSim(this);
	    dskcon.write(addr - 0xFF04, x);
	}
	else
	    mc6809::write(addr, x);
	}
}


static Console sys;


#ifdef SIGALRM
#ifdef sun
void update(int, ...)
#else
void update(int)
#endif
{
	sys.status();
	(void)signal(SIGALRM, update);
	alarm(1);
}
#endif // SIGALRM

int main(int argc, char *argv[])
{
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		fprintf(stderr, "Usage: usim <hexfile|srecfile> [--binary] [<hex load offset>]\n");
		fprintf(stderr, "--binary turns off carriage return to line feed translation on output to $FF00.\n");
		return EXIT_FAILURE;
	}

	//(void)signal(SIGINT, SIG_IGN);
#if 0  /*def SIGALRM*/
	(void)signal(SIGALRM, update);
	alarm(1);
#endif

	const char *progFilename = NULL;
	Word loadOffset = 0;
	bool gotLoadOffset = false;
	for (int argi = 1; argi < argc; ++argi) {
	if (!strcmp(argv[argi], "--binary")) {
		sys.setBinaryMode(true);
	} else if (argv[argi][0] == '-') {
		fprintf(stderr, "usim: Invalid option %s\n", argv[argi]);
		return EXIT_FAILURE;
	} else {  // non-option argument:
		if (!progFilename)
			progFilename = argv[argi];
		else if (!gotLoadOffset) {	// load offset:
			char *end = NULL;
			errno = 0;
			long n = strtol(argv[argi], &end, 16);
			if (errno != 0 || n < 0 || n > 0xFFFF) {
				fprintf(stderr, "usim: invalid load offset %s\n", argv[argi]);
				return EXIT_FAILURE;
			}
			loadOffset = Word(n);
			gotLoadOffset = true;
		}
		else {
			fprintf(stderr, "usim: Invalid argument %s\n", argv[argi]);
			return EXIT_FAILURE;
		}
	}
	}

	if (strstr(progFilename, ".srec") == progFilename + strlen(progFilename) - 5)
		sys.load_srec(progFilename, loadOffset);
	else
		sys.load_intelhex(progFilename, loadOffset);
	sys.run();

	return EXIT_SUCCESS;
}
