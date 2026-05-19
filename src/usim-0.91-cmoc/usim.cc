//
//
//	usim.cc
//
//	(C) R.P.Bellis 1994
//
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <errno.h>
#include "usim.h"

inline long long getCurrentTimeInMicroseconds()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
}

//----------------------------------------------------------------------------
// Generic processor run state routines
//----------------------------------------------------------------------------
void USim::run(void)
{
	halted = 0;

	double timeOfLastIRQ = getCurrentTimeInMicroseconds();

	while (!halted) {
		execute();

		double now = getCurrentTimeInMicroseconds();
		if (now - timeOfLastIRQ >= 1000000 / 60 )  // 60 Hz IRQ
		{
			trigger_irq();
			timeOfLastIRQ = now;
		}
	}
	status();
}

void USim::step(void)
{
	execute();
	status();
}

void USim::halt(void)
{
	halted = 1;
}

Byte USim::fetch(void)
{
	Byte		val = read(pc);
	pc += 1;

	return val;
}

Word USim::fetch_word(void)
{
	Word		val = read_word(pc);
	pc += 2;

	return val;
}

void USim::invalid(const char *msg)
{
	if (ir != 0x0013)  // use SYNC instruction to leave the simulator
		fprintf(stderr, "\r\ninvalid %s : pc = [%04x], ir = [%04x]\r\n",
			msg ? msg : "",
			pc, ir);
	halt();
}

//----------------------------------------------------------------------------
// Primitive (byte) memory access routines
//----------------------------------------------------------------------------

// Single byte read
Byte USim::read(Word offset)
{
	return memory[offset];
}

// Single byte write
void USim::write(Word offset, Byte val)
{
	memory[offset] = val;
}

//----------------------------------------------------------------------------
// Processor loading routines
//----------------------------------------------------------------------------
static Byte fread_byte(FILE *fp)
{
	char			str[3];
	long			l;

	str[0] = fgetc(fp);
	str[1] = fgetc(fp);
	str[2] = '\0';

	l = strtol(str, NULL, 16);
	return (Byte)(l & 0xff);
}

static Word fread_word(FILE *fp)
{
	Word		ret;

	ret = fread_byte(fp);
	ret <<= 8;
	ret |= fread_byte(fp);

	return ret;
}

void USim::load_intelhex(const char *filename, Word loadOffset)
{
	FILE		*fp;
	int		done = 0;

	fp = fopen(filename, "r");
	if (!fp) {
		int e = errno;
		fprintf(stderr, "%s: %s\n", filename, strerror(e));
		exit(EXIT_FAILURE);
	}

	while (!done) {
		Byte		n, t;
		Word		addr;
		Byte		b;

		(void)fgetc(fp);
		n = fread_byte(fp);
		addr = fread_word(fp);
		t = fread_byte(fp);
		if (t == 0x00) {
			while (n--) {
				b = fread_byte(fp);
				memory[loadOffset + addr++] = b;
			}
		} else if (t == 0x01) {
			pc = loadOffset + addr;
			done = 1;
		}
		// Read and discard checksum byte
		(void)fread_byte(fp);
		if (fgetc(fp) == '\r') (void)fgetc(fp);
	}

	fclose(fp);
}


// https://en.wikipedia.org/wiki/SREC_(file_format)
//
class SRECReader
{
public:

    SRECReader()
    :   lineNo(1),
        filename("")
    {
    }

    bool read(FILE *fp, const char* _filename, Byte *memory, Word loadOffset, Word &pc)
    {
        filename = _filename;
        bool haveStart = false;

        char line[512];
        while (fgets(line, sizeof(line), fp))
        {
            if (line[0] != 'S')
                return fail("expecting S at start of line");
            if (line[1] == '1')  // data line
            {
                Byte byteCount = decodeByte(line + 2);
                if ((Word) strlen(line) < 4 + byteCount * 2)
                    return fail("S1 (data) record too short");
                Word addr = decodeWord(line + 4);
                Byte dataByteCount = byteCount - 3;
                for (Word i = 0; i < dataByteCount; ++i)
                {
                    Byte dataByte = decodeByte(line + 8 + i * 2);
                    memory[loadOffset + addr + i] = dataByte;
                }
            }
            else if (line[1] == '9')  // start address
            {
                if (haveStart)
                    return fail("more than one S9 record");
                if (strlen(line) < 8)
                    return fail("S9 (start address) record too short");
                pc = decodeWord(line + 4);
                haveStart = true;
            }
            // Other types of records are ignored.

            ++lineNo;
        }
        return true;  // success
    }

private:

    bool fail(const char *msg)
    {
        printf("%s:%u: %s\n", filename, lineNo, msg);
        return false;
    }

    static Word decodeWord(const char *p)
    {
        return ((Word) decodeByte(p) << 8) | decodeByte(p + 2);
    }

    static Byte decodeByte(const char *p)
    {
        return (decodeNybble(p[0]) << 4) | decodeNybble(p[1]);
    }

    static Byte decodeNybble(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        c = toupper(c);
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return 0;
    }

private:

    unsigned lineNo;
    const char *filename;

};


void USim::load_srec(const char *filename, Word loadOffset)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        int e = errno;
        fprintf(stderr, "%s: %s\n", filename, strerror(e));
        exit(EXIT_FAILURE);
    }

    SRECReader reader;
    bool success = reader.read(fp, filename, memory, loadOffset, pc);

    fclose(fp);

    if (!success)
        exit(EXIT_FAILURE);
}

//----------------------------------------------------------------------------
// Word memory access routines for big-endian (Motorola type)
//----------------------------------------------------------------------------

Word USimMotorola::read_word(Word offset)
{
	Word		tmp;

	tmp = read(offset++);
	tmp <<= 8;
	tmp |= read(offset);

	return tmp;
}

void USimMotorola::write_word(Word offset, Word val)
{
	write(offset++, (Byte)(val >> 8));
	write(offset, (Byte)val);
}

//----------------------------------------------------------------------------
// Word memory access routines for little-endian (Intel type)
//----------------------------------------------------------------------------

Word USimIntel::read_word(Word offset)
{
	Word		tmp;

	tmp = read(offset++);
	tmp |= (read(offset) << 8);

	return tmp;
}

void USimIntel::write_word(Word offset, Word val)
{
	write(offset++, (Byte)val);
	write(offset, (Byte)(val >> 8));
}
