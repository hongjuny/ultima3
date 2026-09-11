#ifndef U3_PARTY_RECORD_H
#define U3_PARTY_RECORD_H

/* PRTY resources contain 64 bytes. Legacy game code addresses them at 1..64;
   index zero is padding, not part of the on-disk record. */
enum { U3PartyResourceSize = 64, U3LegacyPartySize = U3PartyResourceSize + 1 };
extern unsigned char Party[U3LegacyPartySize];

#endif
