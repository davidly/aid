#pragma once

// Taken from https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/set-se-translator?view=msvc-160

#include <eh.h>
#include <exception>

class SE_Exception : public std::exception
{
    private:
        const unsigned int nSE;

    public:
        SE_Exception() noexcept : SE_Exception{ 0 } {}
        SE_Exception( unsigned int n ) noexcept : nSE{ n } {}
        unsigned int getSeNumber() const noexcept { return nSE; }
};

class Scoped_SE_Translator
{
    private:
        const _se_translator_function old_SE_translator;

    public:
        Scoped_SE_Translator( _se_translator_function new_SE_translator ) noexcept
            : old_SE_translator{ _set_se_translator( new_SE_translator ) } {}

        ~Scoped_SE_Translator() noexcept { _set_se_translator( old_SE_translator ); }
};

inline void SE_trans_func( unsigned int u, EXCEPTION_POINTERS* )
{
    throw SE_Exception( u );
}

