#include "sn.cpp"
#include <cstdio>

int main(int argc, char* argv[])
{
    char const* s1 = "ala bala";
    char const* s2 = "ala b2la";
    char const* s3 = "ala b3la";
    char const* s4 = "ala b4la";
    char const* s5 = "ala b5la";
    char const* s6 = "ala b6la";
    char const* s7 = "ala b7la";

    short sn1 = SN_assign(s1);
    short sn2 = SN_assign(s2);
    short sn3 = SN_assign(s3);
    short sn4 = SN_assign(s4);
    short sn5 = SN_assign(s5);
    short sn6 = SN_assign(s6);
    short sn7 = SN_assign(s7);

    printf("s1: %d -> %s\n", (int)sn1, SN_get(sn1));
    printf("s2: %d -> %s\n", (int)sn2, SN_get(sn2));
    printf("s3: %d -> %s\n", (int)sn3, SN_get(sn3));
    printf("s4: %d -> %s\n", (int)sn4, SN_get(sn4));
    printf("s5: %d -> %s\n", (int)sn5, SN_get(sn5));
    printf("s6: %d -> %s\n", (int)sn6, SN_get(sn6));
    printf("s7: %d -> %s\n", (int)sn7, SN_get(sn7));

    SN_dispose(sn2);
    printf("null: '%s'\n", SN_get(sn2));
    short sn8 = SN_assign(s2);
    printf("fill hole: %d\n", (int)sn8);

    SN_dispose(sn2);
    SN_dispose(sn4);
    SN_dispose(sn3);
    printf("0 -> %s\n", SN_get(sn1));
    printf("6 -> %s\n", SN_get(sn7));
    short sn9 = SN_assign(s2);
    short sn10 = SN_assign(s3);
    short sn11 = SN_assign(s4);
    short sn14 = SN_assign(s1);
    printf("%d %d %d\n", (int)sn9, (int)sn10, (int)sn11);
    printf("%s;%s;%s\n", SN_get(sn9), SN_get(sn10), SN_get(sn11));
    printf("%d -> %s\n", (int)sn14, SN_get(sn14));
    SN_dispose(sn14);

    SN_dispose(sn7);
    printf("null: '%s'\n", SN_get(sn7));
    short sn12 = SN_assign(s7);
    printf("%d -> %s\n", (int)sn12, SN_get(sn12));

    SN_reset();
    printf("'%s'\n", SN_get(sn1));

    return 0;
}
