#include <cxxtest/TestSuite.h>

#include "CommonError.h"
#include "ConstSimpleData.h"
#include "HeapData.h"
#include "MockErrno.h"
#include "RealMalloc.h"
#include "T/stdlib.h"
#include <string.h>

class MockForHeapData :
	public T::Base_malloc,
	public T::Base_free,
    public T::Base_realloc,
    public MockErrno
{
public:
	enum {
		MAX_BUFFER_SIZE = 2048,
	};

	MockForHeapData()
	{
		allocated = false;
		memset( buffer, 0, sizeof(buffer) );
		mallocShouldFail = false;
	}

	char buffer[MAX_BUFFER_SIZE];
	bool allocated;
	size_t lastMallocSize;
	bool mallocShouldFail;

	void *malloc( size_t size )
	{
		TS_ASSERT( !allocated );
		allocated = true;

		lastMallocSize = size;

		if ( mallocShouldFail )
			return 0;

		return buffer;
	}

	void free( void *ptr )
	{
		TS_ASSERT( allocated );
		allocated = false;

		TS_ASSERT_EQUALS( ptr, buffer );

		memset( buffer, 0, sizeof(buffer) );
	}

    void *realloc( void *ptr, size_t size )
    {
        if ( allocated )
        {
            TS_ASSERT_EQUALS( ptr, buffer );
        }
        else
        {
            allocated = true;
        }

        lastMallocSize = size;

        return buffer;
    }
};

class HeapDataTestSuite : public CxxTest::TestSuite
{
public:
	MockForHeapData *mock;

	void setUp()
	{
		TS_ASSERT( mock = new MockForHeapData );
	}

	void tearDown()
	{
		TS_ASSERT_THROWS_NOTHING( delete mock );
	}

	void test_empty_data()
	{
		HeapData data;

		TS_ASSERT_EQUALS( data.data(), (void *)0 );
		TS_ASSERT_EQUALS( data.size(), 0U );
	}

	void test_constructor()
	{
		char buffer[] = { 1, 2, 3, 4, 5 };
		char copyOfBuffer[sizeof(buffer)];
		memcpy( copyOfBuffer, buffer, sizeof(buffer) );

		HeapData *data;

		TS_ASSERT( data = new HeapData( buffer, sizeof(buffer) ) );

		TS_ASSERT( mock->allocated );
		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(buffer) );

		TS_ASSERT_EQUALS( data->size(), sizeof(buffer) );
		TS_ASSERT_DIFFERS( data->data(), buffer );
		TS_ASSERT_SAME_DATA( data->data(), buffer, sizeof(buffer) );

		buffer[1]++;

		TS_ASSERT_EQUALS( data->size(), sizeof(buffer) );
		TS_ASSERT_SAME_DATA( data->data(), copyOfBuffer, sizeof(copyOfBuffer) );

		TS_ASSERT_THROWS_NOTHING( delete data );

		TS_ASSERT( !mock->allocated );
	}

	void test_constructor__new_fails()
	{
		mock->mallocShouldFail = true;

		char buffer[] = { 1, 2, 3, 4, 5 };

        TS_ASSERT_THROWS_EQUALS( new HeapData( buffer, sizeof(buffer) ),
								 const CommonError &e,
								 e.getCode(),
								 CommonError::NOT_ENOUGH_MEMORY );
	}

	void test_assignment_from_const_simple_data()
	{
		char buffer[] = { 1, 2, 3 };

		HeapData heapData;
		heapData = ConstSimpleData( buffer, sizeof(buffer) );

		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(buffer) );
		TS_ASSERT_EQUALS( heapData.size(), sizeof(buffer) );
		TS_ASSERT_SAME_DATA( heapData.data(), buffer, sizeof(buffer) );

		char otherBuffer[] = { 'a', 'b', 'c', 'd', 'e' };

		heapData = ConstSimpleData( otherBuffer, sizeof(otherBuffer) );

		TS_ASSERT_EQUALS( heapData.size(), sizeof(otherBuffer) );
		TS_ASSERT_SAME_DATA( heapData.data(), otherBuffer, sizeof(otherBuffer) );

		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(otherBuffer) );
	}

	void test_assignment_from_other_heap_data()
	{
		char buffer[] = { 1, 2, 3 };

        HeapData *other;

        {
            RealMalloc realMalloc;
            other = new HeapData( buffer, sizeof(buffer) );
        }

        HeapData heapData;
        heapData = *other;

        {
            RealMalloc realMalloc;
            delete other;
        }

		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(buffer) );
		TS_ASSERT_EQUALS( heapData.size(), sizeof(buffer) );
		TS_ASSERT_SAME_DATA( heapData.data(), buffer, sizeof(buffer) );
	}

	void test_constructor_from_other_heap_data()
	{
		char buffer[] = { 1, 2, 3 };

        HeapData *other;

        {
            RealMalloc realMalloc;
            other = new HeapData( buffer, sizeof(buffer) );
        }

        HeapData heapData( *other );

        {
            RealMalloc realMalloc;
            delete other;
        }

        TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(buffer) );
		TS_ASSERT_EQUALS( heapData.size(), sizeof(buffer) );
		TS_ASSERT_SAME_DATA( heapData.data(), buffer, sizeof(buffer) );
    }

	void test_concatenation()
	{
		char buffer1[] = { 1, 2, 3 };
        char buffer2[] = { 'a', 'b', 'c' };

        HeapData heapData;
		heapData = ConstSimpleData( buffer1, sizeof(buffer1) );

		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(buffer1) );
		TS_ASSERT_EQUALS( heapData.size(), sizeof(buffer1) );
		TS_ASSERT_SAME_DATA( heapData.data(), buffer1, sizeof(buffer1) );

        TS_ASSERT_THROWS_NOTHING( heapData += ConstSimpleData( buffer2, sizeof(buffer2) ) );

		char expectedData[] = { 1, 2, 3, 'a', 'b', 'c' };

        TS_ASSERT_EQUALS( heapData.size(), sizeof(expectedData) );
		TS_ASSERT_SAME_DATA( heapData.data(), expectedData, sizeof(expectedData) );

		TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(expectedData) );

        HeapData *otherHeapData;

        {
            RealMalloc realMalloc;
            otherHeapData = new HeapData( buffer2, sizeof(buffer2) );
        }

        TS_ASSERT_THROWS_NOTHING( heapData += *otherHeapData );

        {
            RealMalloc realMalloc;
            delete otherHeapData;
        }

        memset( buffer2, 0, sizeof(buffer2) );

        char expectedData2[] = { 1, 2, 3, 'a', 'b', 'c', 'a', 'b', 'c' };

        TS_ASSERT_EQUALS( heapData.size(), sizeof(expectedData2) );
		TS_ASSERT_SAME_DATA( heapData.data(), expectedData2, sizeof(expectedData2) );

        TS_ASSERT_EQUALS( mock->lastMallocSize, sizeof(expectedData2) );
	}

    void test_constructor_from_const_simple_data()
    {
        char data[] = { 0x01, 0x02, 0x03 };
        ConstSimpleData constData( data, sizeof(data) );

        HeapData *heapData;

        TS_ASSERT( heapData = new HeapData( constData ) );

        TS_ASSERT_EQUALS( heapData->size(), sizeof(data) );
        TS_ASSERT_SAME_DATA( heapData->data(), data, sizeof(data) );

        char copyOfData[sizeof(data)];
        memcpy( copyOfData, data, sizeof(data) );
        data[0]++;

        TS_ASSERT_SAME_DATA( heapData->data(), copyOfData, sizeof(copyOfData) );

        TS_ASSERT_THROWS_NOTHING( delete heapData );
    }

    void test_copy_constructor()
    {
        char data[] = { 0x01, 0x02, 0x03 };
        HeapData *other;

        {
            RealMalloc realMalloc;
            other = new HeapData( data, sizeof(data) );
        }

        HeapData *heapData;

        TS_ASSERT( heapData = new HeapData( *other ) );

        {
            RealMalloc realMalloc;
            delete other;
        }

        TS_ASSERT_EQUALS( heapData->size(), sizeof(data) );
        TS_ASSERT_SAME_DATA( heapData->data(), data, sizeof(data) );

        char copyOfData[sizeof(data)];
        memcpy( copyOfData, data, sizeof(data) );
        data[0]++;

        TS_ASSERT_SAME_DATA( heapData->data(), copyOfData, sizeof(copyOfData) );

        TS_ASSERT_THROWS_NOTHING( delete heapData );
    }

    void test_clear()
	{
		char buffer[] = { 1, 2, 3, 4, 5 };
		char copyOfBuffer[sizeof(buffer)];
		memcpy( copyOfBuffer, buffer, sizeof(buffer) );

		HeapData *data;

		TS_ASSERT( data = new HeapData( buffer, sizeof(buffer) ) );

		TS_ASSERT_EQUALS( data->size(), sizeof(buffer) );
		TS_ASSERT_DIFFERS( data->data(), buffer );
		TS_ASSERT_SAME_DATA( data->data(), buffer, sizeof(buffer) );

        data->clear();

        TS_ASSERT_EQUALS( data->size(), 0U );
        TS_ASSERT_EQUALS( data->data(), (void *)0 );

        buffer[1]++;

        *data = ConstSimpleData( copyOfBuffer, sizeof(copyOfBuffer) );

		TS_ASSERT_EQUALS( data->size(), sizeof(copyOfBuffer) );
		TS_ASSERT_SAME_DATA( data->data(), copyOfBuffer, sizeof(copyOfBuffer) );

		TS_ASSERT_THROWS_NOTHING( delete data );
	}

    void test_equality()
    {
        char data1[] = { 0x01, 0x02, 0x03 };
        char data2[] = { 0x01, 0x02, 0x03 };
        char data3[] = { 0x01, 0x04, 0x03 };
        char data4[] = { 0x01, 0x02, 0x03, 0x04 };

        {
            RealMalloc realMalloc;

            HeapData heap1( data1, sizeof(data1) );
            HeapData heap2( data2, sizeof(data2) );
            HeapData heap3( data3, sizeof(data3) );
            HeapData heap4( data4, sizeof(data4) );

            TS_ASSERT( heap1 == heap1 );
            TS_ASSERT( heap1 == heap2 );
            TS_ASSERT( !( heap1 == heap3 ) );
            TS_ASSERT( !( heap1 == heap4 ) );

            TS_ASSERT( !( heap1 != heap1 ) );
            TS_ASSERT( !( heap1 != heap2 ) );
            TS_ASSERT( heap1 != heap3 );
            TS_ASSERT( heap1 != heap4 );
        }
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
