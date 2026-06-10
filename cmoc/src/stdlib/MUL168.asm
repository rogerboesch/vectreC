        SECTION code

MUL168  EXPORT


* Multiply B by X, unsigned; return result in D; preserve X.
MUL168  PSHS    X,B,A           acc A pushed to create temp byte at ,S
        LDA     2,S             high byte of X
        MUL                     multiply by B
        STB     ,S              keep intermediate result
        LDB     1,S             original B
        LDA     3,S             low byte of X
        MUL
        ADDA    ,S
        LEAS    4,S
        RTS


	ENDSECTION
