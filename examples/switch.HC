I64 describe(I64 x)
{
    switch (x) {
        case:
            "zero\n";
            return 0;
        case:
            "one\n";
            return 1;
        case 4...10:
            "between 4 and 10\n";
            return 2;
        default:
            "something else\n";
            return -1;
    }
}

"x=0: "; describe(0);
"x=1: "; describe(1);
"x=7: "; describe(7);
"x=3: "; describe(3);
return 0;
