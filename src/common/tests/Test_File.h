#include <cxxtest/TestSuite.h>

#include "CommonError.h"
#include "ConstSimpleData.h"
#include "File.h"
#include "HeapData.h"
#include "MString.h"
#include "MStringf.h"
#include "MockErrno.h"
#include "RealMalloc.h"

#include "T/sys/stat.h"
#include "T/unistd.h"

class MockForFile :
	public RealMalloc,
    public MockErrno,
    public T::Base_open,
    public T::Base_write,
    public T::Base_read,
    public T::Base_close,
    public T::Base_stat
{
public:
	MockForFile()
	{
        nextDescriptor = 17;
        openWasCalled = false;
        writeShouldFail = false;
        closeWasCalled = false;
        readShouldFail = false;
        statShouldFail = false;
	}

    String lastPathname;
    int lastFlags;
    int nextDescriptor;
    mode_t lastMode;
    bool openWasCalled;

    int open( const char *pathname, int flags, mode_t mode )
    {
        openWasCalled = true;

        lastPathname = pathname;
        lastFlags = flags;
        lastMode = mode;

        return nextDescriptor;
    }

    bool closeWasCalled;

    int close( int fd )
    {
        TS_ASSERT( openWasCalled );
        TS_ASSERT( !closeWasCalled );

        TS_ASSERT_EQUALS( fd, nextDescriptor );

        closeWasCalled = true;

        return 0;
    }

    HeapData writtenData;
    bool writeShouldFail;

    ssize_t write( int fd, const void *buf, size_t count )
    {
        TS_ASSERT( openWasCalled );
        TS_ASSERT_EQUALS( fd, nextDescriptor );
        TS_ASSERT( ( lastFlags == ( O_CREAT | O_RDWR | O_APPEND ) ) ||
                   ( lastFlags == ( O_CREAT | O_RDWR | O_TRUNC ) ) );

        writtenData += ConstSimpleData( buf, count );

        return writeShouldFail ? count - 1 : count;
    }

    HeapData nextReadData;
    bool readShouldFail;

    ssize_t read( int fd, void *buf, size_t count )
    {
        TS_ASSERT( openWasCalled );
        TS_ASSERT_EQUALS( fd, nextDescriptor );
        TS_ASSERT_EQUALS( lastFlags, O_RDONLY );

        if ( readShouldFail )
        {
            return -1;
        }

        unsigned bytes = std::min( count, (size_t)nextReadData.size() );
        memcpy( buf, nextReadData.data(), bytes );

        HeapData tempData = ConstSimpleData( (char *)(nextReadData.data()) + bytes, nextReadData.size() - bytes );
        nextReadData = tempData;

        return bytes;
    }

    String lastStatPath;
    int nextFileSize;
    bool statShouldFail;
    mode_t nextFileMode;

    int stat( const char *path, struct stat *buf )
    {
        lastStatPath = path;

        memset( buf, 0, sizeof(struct stat) );
        buf->st_size = (off_t)nextFileSize;
        memcpy( &buf->st_mode, &nextFileMode, sizeof(mode_t) );

        return statShouldFail ? -1 : 0;
    }
};

class FileTestSuite : public CxxTest::TestSuite
{
public:
	MockForFile *mock;

	void setUp()
	{
		TS_ASSERT( mock = new MockForFile );
	}

	void tearDown()
	{
		TS_ASSERT_THROWS_NOTHING( delete mock );
	}

    void test_open__write_mode()
    {
        File *file;

        TS_ASSERT( file = new File( "/root/projects/test.txt" ) );

        TS_ASSERT_THROWS_NOTHING( file->open( File::MODE_WRITE_APPEND ) );

        TS_ASSERT( mock->openWasCalled );
        TS_ASSERT( !mock->closeWasCalled );

        TS_ASSERT_EQUALS( mock->lastFlags, O_CREAT | O_APPEND | O_RDWR );
        TS_ASSERT_EQUALS( mock->lastMode, (mode_t)( S_IRUSR | S_IWUSR ) );
        TS_ASSERT_EQUALS( mock->lastPathname, "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( delete file );

        TS_ASSERT( mock->closeWasCalled );
    }

    void test_open__write_truncate_mode()
    {
        File *file;

        TS_ASSERT( file = new File( "/root/projects/test.txt" ) );

        TS_ASSERT_THROWS_NOTHING( file->open( File::MODE_WRITE_TRUNCATE ) );

        TS_ASSERT( mock->openWasCalled );
        TS_ASSERT( !mock->closeWasCalled );

        TS_ASSERT_EQUALS( mock->lastFlags, O_CREAT | O_RDWR | O_TRUNC );
        TS_ASSERT_EQUALS( mock->lastMode, (mode_t)( S_IRUSR | S_IWUSR ) );
        TS_ASSERT_EQUALS( mock->lastPathname, "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( delete file );

        TS_ASSERT( mock->closeWasCalled );
    }

    void test_open__read_mode()
    {
        File *file;

        TS_ASSERT( file = new File( "/root/projects/test.txt" ) );

        TS_ASSERT_THROWS_NOTHING( file->open( File::MODE_READ ) );

        TS_ASSERT( mock->openWasCalled );
        TS_ASSERT( !mock->closeWasCalled );

        TS_ASSERT_EQUALS( mock->lastFlags, O_RDONLY );
        TS_ASSERT_EQUALS( mock->lastPathname, "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( delete file );

        TS_ASSERT( mock->closeWasCalled );
    }

    void test_open__fail()
    {
        File *file;

        TS_ASSERT( file = new File( "/root/projects/test.txt" ) );

        mock->nextDescriptor = -1;

        TS_ASSERT_THROWS_EQUALS( file->open( File::MODE_WRITE_TRUNCATE ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::OPEN_FAILED );

        TS_ASSERT_THROWS_NOTHING( delete file );

        TS_ASSERT( !mock->closeWasCalled );
    }

    void test_write()
    {
        File file( "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( file.open( File::MODE_WRITE_TRUNCATE ) );

        String line1 = "this is a line";
        String line2 = "this is its friend";

        TS_ASSERT_THROWS_NOTHING( file.write( line1 ) );
        TS_ASSERT_THROWS_NOTHING( file.write( line2 ) );

        String expected = line1 + line2;

        TS_ASSERT_EQUALS( mock->writtenData.size(), expected.length() );
        TS_ASSERT_SAME_DATA( mock->writtenData.data(), expected.ascii(), expected.length() );
    }

    void test_write__fails()
    {
        File file( "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( file.open( File::MODE_WRITE_TRUNCATE ) );

        String line1 = "this is a line";
        String line2 = "this is its friend";

        mock->writeShouldFail = true;

        TS_ASSERT_THROWS_EQUALS( file.write( line1 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::WRITE_FAILED );
    }

    void test_read()
    {
        File file( "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( file.open( File::MODE_READ ) );

        char toRead[] = "hello world, im a buffer";

        mock->nextReadData = ConstSimpleData( toRead, sizeof(toRead) );

        HeapData readData1;

        file.read( readData1, 5 );
        TS_ASSERT_EQUALS( readData1.size(), 5U );
        TS_ASSERT_SAME_DATA( readData1.data(), toRead, 5 );

        mock->nextReadData = ConstSimpleData( toRead, sizeof(toRead) );

        HeapData readData2;

        file.read( readData2, sizeof(toRead) );
        TS_ASSERT_EQUALS( readData2.size(), sizeof(toRead) );
        TS_ASSERT_SAME_DATA( readData2.data(), toRead, sizeof(toRead) );

        mock->nextReadData = ConstSimpleData( toRead, sizeof(toRead) );

        HeapData readData3;

        file.read( readData3, sizeof(toRead) + 17 );
        TS_ASSERT_EQUALS( readData3.size(), sizeof(toRead) );
        TS_ASSERT_SAME_DATA( readData3.data(), toRead, sizeof(toRead) );

        mock->writeShouldFail = true;
    }

    void test_read__fails()
    {
        File file( "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( file.open( File::MODE_READ ) );

        HeapData readData1;

        mock->readShouldFail = true;

        TS_ASSERT_THROWS_EQUALS( file.read( readData1, 5 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::READ_FAILED );
    }

    void test_readline()
    {
        File file( "/root/projects/test.txt" );

        TS_ASSERT_THROWS_NOTHING( file.open( File::MODE_READ ) );

        char toRead1[] = "As I was going to St Ives\n";
        char toRead2[] = "I met a man with seven wives\n";
        char toRead3[] = "Every wife had seven sacks\n";
        char toRead4[] = "Every sack had seven cats\n";
        char toRead5[] = "Every cat had seven kittens\n";
        char toRead6[] = "Kittens, cats, sacks and wives, how many were going to St Ives?\n";

        Stringf all( "%s%s%s%s%s%s", toRead1, toRead2, toRead3, toRead4, toRead5, toRead6 );
        mock->nextReadData = ConstSimpleData( all.ascii(), all.length() );

        String line;
        String expectedLine;

        line = file.readLine();
        expectedLine = "As I was going to St Ives";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file.readLine();
        expectedLine = "I met a man with seven wives";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file.readLine();
        expectedLine = "Every wife had seven sacks";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file.readLine();
        expectedLine = "Every sack had seven cats";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file.readLine();
        expectedLine = "Every cat had seven kittens";
        TS_ASSERT_EQUALS( line, expectedLine );

        line = file.readLine();
        expectedLine = "Kittens, cats, sacks and wives, how many were going to St Ives?";
        TS_ASSERT_EQUALS( line, expectedLine );

        TS_ASSERT_THROWS_EQUALS( file.readLine(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::READ_FAILED );
    }

    void test_size()
    {
        mock->nextFileSize = 5523;

        TS_ASSERT_EQUALS( File::getSize( "/home/user/bla" ), 5523U );
        TS_ASSERT_EQUALS( mock->lastStatPath, "/home/user/bla" );

        mock->statShouldFail = true;

        TS_ASSERT_THROWS_EQUALS( File::getSize( "test/test.a" ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::STAT_FAILED );
    }

    void test_exists()
    {
        mock->nextFileSize = 5523;

        TS_ASSERT_EQUALS( File::getSize( "/home/user/bla" ), 5523U );
        TS_ASSERT_EQUALS( mock->lastStatPath, "/home/user/bla" );

        mock->statShouldFail = false;
        TS_ASSERT( File::exists( "test/test.a" ) );

        mock->statShouldFail = true;
        TS_ASSERT( !File::exists( "test/test.a" ) );
    }

    void test_folder()
    {
        memset( &mock->nextFileMode, 0, sizeof(mode_t) );

        mock->statShouldFail = false;

        mock->nextFileMode = S_IFREG;
        TS_ASSERT( !File::directory( "/test/bla" ) );

        mock->nextFileMode = S_IFDIR;
        TS_ASSERT( File::directory( "/test/bla" ) );

        mock->statShouldFail = true;

        TS_ASSERT( !File::directory( "/test/bla" ) );
    }

    void test_close()
    {
        File *file;

        TS_ASSERT( file = new File( "/root/projects/test.txt" ) );

        TS_ASSERT_THROWS_NOTHING( file->open( File::MODE_WRITE_APPEND ) );

        TS_ASSERT( mock->openWasCalled );
        TS_ASSERT( !mock->closeWasCalled );

        TS_ASSERT_THROWS_NOTHING( file->close() );

        TS_ASSERT( mock->closeWasCalled );

        TS_ASSERT_THROWS_NOTHING( delete file );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
