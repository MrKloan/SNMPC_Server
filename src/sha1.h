#ifndef SHA1_H_INCLUDED
#define SHA1_H_INCLUDED

/*
 *  Function Prototypes
 */
void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *, const unsigned char *, unsigned);

#endif // SHA1_H_INCLUDED
