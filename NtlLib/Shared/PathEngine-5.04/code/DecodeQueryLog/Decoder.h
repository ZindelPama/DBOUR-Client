//**********************************************************************
//
// Copyright (c) 2005
// PathEngine
// Lyon, France
//
// All Rights Reserved
//
//**********************************************************************

#ifndef DECODE_QUERY_LOG__DECODER_INCLUDED
#define DECODE_QUERY_LOG__DECODER_INCLUDED

#include "common/STL/string.h"

class iXMLOutputStream;

class cDecoder
{
    const char* _bufferPosition;
    unsigned long _remainingSize;
    bool _overran;
    bool _methodEntry;
    iXMLOutputStream& _os;

    long decodeSigned(long tokenSize);
    unsigned long decodeUnsigned(long tokenSize);
    bool decodeBool();
    float decodeFloat();
    std::string decodeString();

    void decodeTimeStamp();

    void decodeMethodInfo(long& interfaceIndex, long& methodIndex, bool& methodEntry);

public:

    cDecoder(
        const char* buffer, unsigned long bufferLength, iXMLOutputStream& os,
        long& interfaceIndex, long& methodIndex, bool& methodEntry,
        unsigned long& recordSize
        );

    bool overran() const;
    bool empty() const;

    void writeInterfaceName(const char* name);
    void writeMethodName(const char* name);

    void decode_bool(const char* name);
    void decode_long(const char* name);
    void decode_unsigned_long(const char* name);
    void decode_float(const char* name);
    void decode_const_char_star(const char* name);
    void decode_const_char__starconst_star(const char* name);
    void decode_const_long_star(const char* name);
    void decode_const_float_star(const char* name);
    void decode_void_star(const char* name);
    void decode_cPosition(const char* name);
    void decodeAPIObject(const char* name);
    void decodeCallBack(const char* name);
    void decodeClassCallBack(const char* name);
    void decodeCollidingLine();
    void decodeCollisionShape();
    void decodeDataBuffer();
    void decodePathBuffers();
    void decodeSourceContent();
    void decodeAgentVector();

    void writeRecordEnd();
};

#endif
