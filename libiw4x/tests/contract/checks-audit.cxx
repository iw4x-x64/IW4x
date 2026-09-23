// Contract checks with audit checking.
//
#undef  LIBIW4X_CONTRACT
#define LIBIW4X_CONTRACT 2 // LIBIW4X_CONTRACT_AUDIT

#define TEST_LEVEL_ID   level_audit
#define TEST_LEVEL_NAME "audit"

#include "checks-impl.hxx"
