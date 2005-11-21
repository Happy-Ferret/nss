/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Sun Microsystems
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * pkix_pl_ldaprequest.c
 *
 */

#include "pkix_pl_ldaprequest.h"

/* --Private-LdapRequest-Functions------------------------------------- */

/* Note: lengths do not include the NULL terminator */
static const char caAttr[] = "caCertificate;binary";
static unsigned int caAttrLen = sizeof(caAttr) - 1;
static const char uAttr[] = "userCertificate;binary";
static unsigned int uAttrLen = sizeof(uAttr) - 1;
static const char ccpAttr[] = "crossCertificatePair;binary";
static unsigned int ccpAttrLen = sizeof(ccpAttr) - 1;
static const char crlAttr[] = "certificateRevocationList;binary";
static unsigned int crlAttrLen = sizeof(crlAttr) - 1;
static const char arlAttr[] = "authorityRevocationList;binary";
static unsigned int arlAttrLen = sizeof(arlAttr) - 1;

/*
 * FUNCTION: pkix_pl_LdapRequest_AttrTypeToBit
 * DESCRIPTION:
 *
 *  This function creates an attribute mask bit corresponding to the SECItem
 *  pointed to by "attrType", storing the result at "pAttrBit". The comparison
 *  is case-insensitive. If "attrType" does not match any of the known types,
 *  zero is stored at "pAttrBit".
 *
 * PARAMETERS
 *  "attrType"
 *      The address of the SECItem whose string contents are to be compared to
 *      the various known attribute types. Must be non-NULL.
 *  "pAttrBit"
 *      The address where the result is stored. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapRequest_AttrTypeToBit(
        SECItem *attrType,
        LdapAttrMask *pAttrBit,
        void *plContext)
{
        LdapAttrMask attrBit = 0;
        unsigned int attrLen = 0;
        const char *s = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_AttrTypeToBit");
        PKIX_NULLCHECK_TWO(attrType, pAttrBit);

        s = (const char *)attrType->data;
        attrLen = attrType->len;

        /*
         * Taking note of the fact that all of the comparand strings are
         * different lengths, we do a slight optimization. If a string
         * length matches but the string does not match, we skip comparing
         * to the other strings. If new strings are added to the comparand
         * list, and any are of equal length, be careful to change the
         * grouping of tests accordingly.
         */
        if (attrLen == caAttrLen) {
                if (PORT_Strncasecmp(caAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CACERT;
                }
        } else if (attrLen == uAttrLen) {
                if (PORT_Strncasecmp(uAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_USERCERT;
                }
        } else if (attrLen == ccpAttrLen) {
                if (PORT_Strncasecmp(ccpAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CROSSPAIRCERT;
                }
        } else if (attrLen == crlAttrLen) {
                if (PORT_Strncasecmp(crlAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_CERTREVLIST;
                }
        } else if (attrLen == arlAttrLen) {
                if (PORT_Strncasecmp(arlAttr, s, attrLen) == 0) {
                        attrBit = LDAPATTR_AUTHREVLIST;
                }
        }

        *pAttrBit = attrBit;
cleanup:

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_EncodeAttrs
 * DESCRIPTION:
 *
 *  This function obtains the attribute mask bits from the LdapRequest pointed
 *  to by "request", creates the corresponding array of AttributeTypes for the
 *  encoding of the SearchRequest message.
 *
 * PARAMETERS
 *  "request"
 *      The address of the LdapRequest whose attributes are to be encoded. Must
 *      be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapRequest_EncodeAttrs(
        PKIX_PL_LdapRequest *request,
        void *plContext)
{
        SECItem **attrArray = NULL;
        PKIX_UInt32 attrIndex = 0;
        LdapAttrMask attrBits;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_EncodeAttrs");
        PKIX_NULLCHECK_ONE(request);

        /* construct "attrs" according to bits in request->attrBits */
        attrBits = request->attrBits;
        attrArray = request->attrArray;
        if ((attrBits & LDAPATTR_CACERT) == LDAPATTR_CACERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)caAttr;
                request->attributes[attrIndex].len = caAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_USERCERT) == LDAPATTR_USERCERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)uAttr;
                request->attributes[attrIndex].len = uAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_CROSSPAIRCERT) == LDAPATTR_CROSSPAIRCERT) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)ccpAttr;
                request->attributes[attrIndex].len = ccpAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_CERTREVLIST) == LDAPATTR_CERTREVLIST) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)crlAttr;
                request->attributes[attrIndex].len = crlAttrLen;
                attrIndex++;
        }
        if ((attrBits & LDAPATTR_AUTHREVLIST) == LDAPATTR_AUTHREVLIST) {
                attrArray[attrIndex] = &(request->attributes[attrIndex]);
                request->attributes[attrIndex].type = siAsciiString;
                request->attributes[attrIndex].data = (unsigned char *)arlAttr;
                request->attributes[attrIndex].len = arlAttrLen;
                attrIndex++;
        }
        attrArray[attrIndex] = (SECItem *)NULL;

cleanup:

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_Destroy
 * (see comments for PKIX_PL_DestructorCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapRequest_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_LdapRequest *ldapRq = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPREQUEST_TYPE, plContext),
                    "Object is not a LdapRequest");

        ldapRq = (PKIX_PL_LdapRequest *)object;

        /*
         * All dynamic field in an LDAPRequest are allocated
         * in an arena, and will be freed when the arena is destroyed.
         */

cleanup:

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_Hashcode
 * (see comments for PKIX_PL_HashcodeCallback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapRequest_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 dataLen = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 idLen = 0;
        const unsigned char *msgBuf = NULL;
        PKIX_PL_LdapRequest *ldapRq = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_LDAPREQUEST_TYPE, plContext),
                    "Object is not a LdapRequest");

        ldapRq = (PKIX_PL_LdapRequest *)object;

        *pHashcode = 0;

        /*
         * Two requests that differ only in msgnum are a match! Therefore,
         * start hashcoding beyond the encoded messageID field.
         */
        if (ldapRq->encoded) {
                msgBuf = (const unsigned char *)ldapRq->encoded->data;
                /* Is message length short form (one octet) or long form? */
                if ((msgBuf[1] & 0x80) != 0) {
                        sizeOfLength = msgBuf[1] & 0x7F;
                        for (dindex = 0; dindex < sizeOfLength; dindex++) {
                                dataLen = (dataLen << 8) + msgBuf[dindex + 2];
                        }
                } else {
                        dataLen = msgBuf[1];
                }

                /* How many bytes for the messageID? (Assume short form) */
                idLen = msgBuf[dindex + 3] + 2;
                dindex += idLen;
                dataLen -= idLen;
                msgBuf = &msgBuf[dindex + 2];

                PKIX_CHECK(pkix_hash(msgBuf, dataLen, pHashcode, plContext),
                        "pkix_hash failed");
        }

cleanup:

        PKIX_RETURN(LDAPREQUEST);

}

/*
 * FUNCTION: pkix_pl_LdapRequest_Equals
 * (see comments for PKIX_PL_Equals_Callback in pkix_pl_system.h)
 */
static PKIX_Error *
pkix_pl_LdapRequest_Equals(
        PKIX_PL_Object *firstObj,
        PKIX_PL_Object *secondObj,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_LdapRequest *firstReq = NULL;
        PKIX_PL_LdapRequest *secondReq = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_UInt32 firstLen = 0;
        const unsigned char *firstData = NULL;
        const unsigned char *secondData = NULL;
        PKIX_UInt32 sizeOfLength = 0;
        PKIX_UInt32 dindex = 0;
        PKIX_UInt32 i = 0;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Equals");
        PKIX_NULLCHECK_THREE(firstObj, secondObj, pResult);

        /* test that firstObj is a LdapRequest */
        PKIX_CHECK(pkix_CheckType(firstObj, PKIX_LDAPREQUEST_TYPE, plContext),
                    "firstObj argument is not a LdapRequest");

        /*
         * Since we know firstObj is a LdapRequest, if both references are
         * identical, they must be equal
         */
        if (firstObj == secondObj){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        /*
         * If secondObj isn't a LdapRequest, we don't throw an error.
         * We simply return a Boolean result of FALSE
         */
        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObj, &secondType, plContext),
                    "Could not get type of second argument");
        if (secondType != PKIX_LDAPREQUEST_TYPE) {
                goto cleanup;
        }

        firstReq = (PKIX_PL_LdapRequest *)firstObj;
        secondReq = (PKIX_PL_LdapRequest *)secondObj;

        /* If either lacks an encoded string, they cannot be compared */
        if (!(firstReq->encoded) || !(secondReq->encoded)) {
                goto cleanup;
        }

        if (firstReq->encoded->len != secondReq->encoded->len) {
                goto cleanup;
        }

        firstData = (const unsigned char *)firstReq->encoded->data;
        secondData = (const unsigned char *)secondReq->encoded->data;

        /*
         * Two requests that differ only in msgnum are equal! Therefore,
         * start the byte comparison beyond the encoded messageID field.
         */

        /* Is message length short form (one octet) or long form? */
        if ((firstData[1] & 0x80) != 0) {
                sizeOfLength = firstData[1] & 0x7F;
                for (dindex = 0; dindex < sizeOfLength; dindex++) {
                        firstLen = (firstLen << 8) + firstData[dindex + 2];
                }
        } else {
                firstLen = firstData[1];
        }

        /* How many bytes for the messageID? (Assume short form) */
        i = firstData[dindex + 3] + 2;
        dindex += i;
        firstLen -= i;
        firstData = &firstData[dindex + 2];

        /*
         * In theory, we have to calculate where the second message data
         * begins by checking its length encodings. But if these messages
         * are equal, we can re-use the calculation we already did. If they
         * are not equal, the byte comparisons will surely fail.
         */

        secondData = &secondData[dindex + 2];
        
        for (i = 0; i < firstLen; i++) {
                if (firstData[i] != secondData[i]) {
                        goto cleanup;
                }
        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_RegisterSelf
 * DESCRIPTION:
 *  Registers PKIX_LDAPREQUEST_TYPE and its related functions with
 *  systemClasses[]
 * PARAMETERS:
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Not Thread Safe - for performance and complexity reasons
 *
 *  Since this function is only called by PKIX_PL_Initialize, which should
 *  only be called once, it is acceptable that this function is not
 *  thread-safe.
 */
PKIX_Error *
pkix_pl_LdapRequest_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_RegisterSelf");

        entry.description = "LdapRequest";
        entry.destructor = pkix_pl_LdapRequest_Destroy;
        entry.equalsFunction = pkix_pl_LdapRequest_Equals;
        entry.hashcodeFunction = pkix_pl_LdapRequest_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_LDAPREQUEST_TYPE] = entry;

        PKIX_RETURN(LDAPREQUEST);
}

/* --Public-Functions------------------------------------------------------- */

/*
 * FUNCTION: pkix_pl_LdapRequest_Create
 * DESCRIPTION:
 *
 *  This function creates an LdapRequest using the PRArenaPool pointed to by
 *  "arena", a message number whose value is "msgnum", a base object pointed to
 *  by "issuerDN", a scope whose value is "scope", a derefAliases flag whose
 *  value is "derefAliases", a sizeLimit whose value is "sizeLimit", a timeLimit
 *  whose value is "timeLimit", an attrsOnly flag whose value is "attrsOnly", a
 *  filter whose value is "filter", and attribute bits whose value is
 *  "attrBits"; storing the result at "pRequestMsg".
 *
 *  See pkix_pl_ldaptemplates.c (and below) for the ASN.1 representation of
 *  message components, and see pkix_pl_ldapt.h for data types.
 *
 * PARAMETERS
 *  "arena"
 *      The address of the PRArenaPool to be used in the encoding. Must be
 *      non-NULL.
 *  "msgnum"
 *      The UInt32 message number to be used for the messageID component of the
 *      LDAP message exchange.
 *  "issuerDN"
 *      The address of the string to be used for the baseObject component of the
 *      LDAP SearchRequest message. Must be non-NULL.
 *  "scope"
 *      The (enumerated) ScopeType to be used for the scope component of the
 *      LDAP SearchRequest message
 *  "derefAliases"
 *      The (enumerated) DerefType to be used for the derefAliases component of
 *      the LDAP SearchRequest message
 *  "sizeLimit"
 *      The UInt32 value to be used for the sizeLimit component of the LDAP
 *      SearchRequest message
 *  "timeLimit"
 *      The UInt32 value to be used for the timeLimit component of the LDAP
 *      SearchRequest message
 *  "attrsOnly"
 *      The Boolean value to be used for the attrsOnly component of the LDAP
 *      SearchRequest message
 *  "filter"
 *      The filter to be used for the filter component of the LDAP
 *      SearchRequest message
 *  "attrBits"
 *      The LdapAttrMask bits indicating the attributes to be included in the
 *      attributes sequence of the LDAP SearchRequest message
 *  "pRequestMsg"
 *      The address at which the address of the LdapRequest is stored. Must
 *      be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
/*
 * SearchRequest ::=
 *      [APPLICATION 3] SEQUENCE {
 *              baseObject      LDAPDN,
 *              scope           ENUMERATED {
 *                                      baseObject              (0),
 *                                      singleLevel             (1),
 *                                      wholeSubtree            (2)
 *                              },
 *              derefAliases    ENUMERATED {
 *                                      neverDerefAliases       (0),
 *                                      derefInSearching        (1),
 *                                      derefFindingBaseObj     (2),
 *                                      alwaysDerefAliases      (3)
 *                              },
 *              sizeLimit       INTEGER (0 .. MAXINT),
 *                              -- value of 0 implies no sizeLimit
 *              timeLimit       INTEGER (0 .. MAXINT),
 *                              -- value of 0 implies no timeLimit
 *              attrsOnly       BOOLEAN,
 *                              -- TRUE, if only attributes (without values)
 *                              -- to be returned
 *              filter          Filter,
 *              attributes      SEQUENCE OF AttributeType
 *      }
 *
 * Filter ::=
 *      CHOICE {
 *              and             [0] SET OF Filter,
 *              or              [1] SET OF Filter,
 *              not             [2] Filter,
 *              equalityMatch   [3] AttributeValueAssertion,
 *              substrings      [4] SubstringFilter,
 *              greaterOrEqual  [5] AttributeValueAssertion,
 *              lessOrEqual     [6] AttributeValueAssertion,
 *              present         [7] AttributeType,
 *              approxMatch     [8] AttributeValueAssertion
 *      }
 *
 * SubstringFilter ::=
 *      SEQUENCE {
 *              type            AttributeType,
 *              SEQUENCE OF CHOICE {
 *                      initial [0] LDAPString,
 *                      any     [1] LDAPString,
 *                      final   [2] LDAPString,
 *              }
 *      }
 *
 * AttributeValueAssertion ::=
 *      SEQUENCE {
 *              attributeType   AttributeType,
 *              attributeValue  AttributeValue,
 *      }
 *
 * AttributeValue ::= OCTET STRING
 *
 * AttributeType ::= LDAPString
 *               -- text name of the attribute, or dotted
 *               -- OID representation
 *
 * LDAPDN ::= LDAPString
 *
 * LDAPString ::= OCTET STRING
 *
 */
PKIX_Error *
pkix_pl_LdapRequest_Create(
        PRArenaPool *arena,
        PKIX_UInt32 msgnum,
        char *issuerDN,
        ScopeType scope,
        DerefType derefAliases,
        PKIX_UInt32 sizeLimit,
        PKIX_UInt32 timeLimit,
        char attrsOnly,
        LDAPFilter *filter,
        LdapAttrMask attrBits,
        PKIX_PL_LdapRequest **pRequestMsg,
        void *plContext)
{
        LDAPMessage msg;
        LDAPSearch *search;
        PKIX_PL_LdapRequest *ldapRequest = NULL;
        char scopeTypeAsChar;
        char derefAliasesTypeAsChar;
        SECItem *attrArray[MAX_LDAPATTRS + 1];

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_Create");
        PKIX_NULLCHECK_THREE(arena, issuerDN, pRequestMsg);

        /* create a PKIX_PL_LdapRequest object */
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_LDAPREQUEST_TYPE,
                    sizeof (PKIX_PL_LdapRequest),
                    (PKIX_PL_Object **)&ldapRequest,
                    plContext),
                    "Could not create object");

        ldapRequest->arena = arena;
        ldapRequest->msgnum = msgnum;
        ldapRequest->issuerDN = issuerDN;
        ldapRequest->scope = scope;
        ldapRequest->derefAliases = derefAliases;
        ldapRequest->sizeLimit = sizeLimit;
        ldapRequest->timeLimit = timeLimit;
        ldapRequest->attrsOnly = attrsOnly;
        ldapRequest->filter = filter;
        ldapRequest->attrBits = attrBits;

        ldapRequest->attrArray = attrArray;

        PKIX_CHECK(pkix_pl_LdapRequest_EncodeAttrs
                (ldapRequest, plContext),
                "pkix_pl_LdapRequest_EncodeAttrs failed");

        PKIX_PL_NSSCALL
                (LDAPREQUEST, PORT_Memset, (&msg, 0, sizeof (LDAPMessage)));

        msg.messageID.type = siUnsignedInteger;
        msg.messageID.data = (void*)&msgnum;
        msg.messageID.len = sizeof (msgnum);

        msg.protocolOp.selector = LDAP_SEARCH_TYPE;

        search = &(msg.protocolOp.op.searchMsg);

        search->baseObject.type = siAsciiString;
        search->baseObject.data = (void *)issuerDN;
        search->baseObject.len = PL_strlen(issuerDN);
        scopeTypeAsChar = (char)scope;
        search->scope.type = siUnsignedInteger;
        search->scope.data = (void *)&scopeTypeAsChar;
        search->scope.len = sizeof (scopeTypeAsChar);
        derefAliasesTypeAsChar = (char)derefAliases;
        search->derefAliases.type = siUnsignedInteger;
        search->derefAliases.data =
                (void *)&derefAliasesTypeAsChar;
        search->derefAliases.len =
                sizeof (derefAliasesTypeAsChar);
        search->sizeLimit.type = siUnsignedInteger;
        search->sizeLimit.data = (void *)&sizeLimit;
        search->sizeLimit.len = sizeof (PKIX_UInt32);
        search->timeLimit.type = siUnsignedInteger;
        search->timeLimit.data = (void *)&timeLimit;
        search->timeLimit.len = sizeof (PKIX_UInt32);
        search->attrsOnly.type = siBuffer;
        search->attrsOnly.data = (void *)&attrsOnly;
        search->attrsOnly.len = sizeof (attrsOnly);

        PKIX_PL_NSSCALL
                (LDAPREQUEST,
                PORT_Memcpy,
                (&search->filter, filter, sizeof (LDAPFilter)));

        search->attributes = attrArray;

        PKIX_PL_NSSCALLRV
                (LDAPCERTSTORECONTEXT, ldapRequest->encoded, SEC_ASN1EncodeItem,
                (arena, NULL, (void *)&msg, PKIX_PL_LDAPMessageTemplate));

        if (!(ldapRequest->encoded)) {
                PKIX_ERROR("failed in encoding searchRequest");
        }

        *pRequestMsg = ldapRequest;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(ldapRequest);
        }

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_GetEncoded
 * DESCRIPTION:
 *
 *  This function obtains the encoded message from the LdapRequest pointed to
 *  by "request", storing the result at "pRequestBuf".
 *
 * PARAMETERS
 *  "request"
 *      The address of the LdapRequest whose encoded message is to be
 *      retrieved. Must be non-NULL.
 *  "pRequestBuf"
 *      The address at which is stored the address of the encoded message. Must
 *      be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapRequest_GetEncoded(
        PKIX_PL_LdapRequest *request,
        SECItem **pRequestBuf,
        void *plContext)
{
        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_GetEncoded");
        PKIX_NULLCHECK_TWO(request, pRequestBuf);

        *pRequestBuf = request->encoded;

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_ParseTokens
 * DESCRIPTION:
 *
 *  This function parses the string stored at "startPos" into tokens based on
 *  "separtor" as the separator and "terminator" as end of parsing. It returns
 *  an array of pointers that holds those tokens and the array is terminated
 *  with NULL.
 *   *
 * PARAMETERS
 *  "request"
 *      The address of the LdapRequest whose parsedLocation is to be updated.
 *      Must be non-NULL.
 *  "startPos"
 *      The address of char string that contains a subset of ldap location.
 *  "tokens"
 *      The address of an array of char string for storing returned tokens.
 *      Must be non-NULL.
 *  "separator"
 *      The character that is taken as token separator. Must be non-NULL.
 *  "terminator"
 *      The character that is taken as parsing terminator. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
static PKIX_Error *
pkix_pl_LdapRequest_ParseTokens(
        PKIX_PL_LdapRequest *request,
        char **startPos, /* return update */
        char ***tokens,
        char separator,
        char terminator,
        void *plContext)
{
        PKIX_UInt32 len = 0;
        PKIX_UInt32 numFilters = 0;
        PKIX_Int32 cmpResult = -1;
        char *endPos = NULL;
        char *p = NULL;
        char **filterP = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_ParseTokens");
        PKIX_NULLCHECK_THREE(request, startPos, tokens);

        endPos = *startPos;

        /* Firs path: parse to <terminator> to count number of components */
        numFilters = 0;
        while (*endPos != terminator && *endPos != '\0') {
                endPos++;
		if (*endPos == separator) {
                        numFilters++;
                }
        }

        if (*endPos != terminator) {
                PKIX_ERROR("Unexpected LDAP location string format filter/bit");
        }

        /* Last one doesn't have a "," as separator, although we allow it */
        if (*(endPos-1) != ',') {
                numFilters++;
        }

        PKIX_PL_NSSCALLRV
                (LDAPREQUEST, *tokens, PORT_ArenaZAlloc,
                (request->arena, (numFilters+1)*sizeof(void *)));

        /* Second path: parse to fill in components in token array */
        filterP = *tokens;
        endPos = *startPos;

	while (numFilters) {
		if (*endPos == separator || *endPos == terminator) {
                        len = endPos - *startPos;
                        PKIX_PL_NSSCALLRV(LDAPREQUEST, p, PORT_ArenaZAlloc,
                            (request->arena, (len+1)));

                        *filterP = p;

                        while(len) {
                            if (**startPos == '%') {
                            /* replacing %20 by blank */
                                PKIX_PL_NSSCALLRV(LDAPREQUEST, cmpResult,
                                    strncmp, ((void *)*startPos, "%20", 3));
                                if (cmpResult == 0) {
                                    *p = ' ';
                                    *startPos += 3;
                                    len -= 3;
                                }
                            } else {
                                *p = **startPos;
                                (*startPos)++;
                                len--;
                            }
                            p++;
                        }

                        *p = '\0';
                        filterP++;
                        numFilters--;

                        if (endPos == '\0') {
                            break;
                        } else {
                            endPos++;
                            *startPos = endPos;
                            continue;
                        }

                }

                endPos++;
        }

        *filterP = NULL;

cleanup:

        PKIX_RETURN(LDAPREQUEST);
}

/*
 * FUNCTION: pkix_pl_LdapRequest_ParseLocation
 * DESCRIPTION:
 *
 *  This function parses the "location" for the LdapRequest pointed to
 *  by "request", and storing the result at parsedLocation field in "request".
 *  The expected location string should be in the following BNF format:
 *
 *  ldap://<ldap-server-site>/[cn=<cname>][,o=<org>][,c=<country>]?
 *  [caCertificate|crossCertificatPair|certificateRevocationList];
 *  [binary|<other-type>]
 *  [[,caCertificate|crossCertificatPair|certificateRevocationList]
 *   [binary|<other-type>]]*
 *
 *
 * PARAMETERS
 *  "request"
 *      The address of the LdapRequest whose LDAP location is to be
 *      parsed. Must be non-NULL.
 *  "location"
 *      The address of char that contains the ldap location. Must be non-NULL.
 *  "plContext"
 *      Platform-specific context pointer.
 * THREAD SAFETY:
 *  Thread Safe (see Thread Safety Definitions in Programmer's Guide)
 * RETURNS:
 *  Returns NULL if the function succeeds.
 *  Returns an LdapRequest Error if the function fails in a non-fatal way.
 *  Returns a Fatal Error if the function fails in an unrecoverable way.
 */
PKIX_Error *
pkix_pl_LdapRequest_ParseLocation(
        PKIX_PL_LdapRequest *request,
        PKIX_PL_GeneralName *location,
        void *plContext)
{
        struct pkix_pl_LdapLocation *parsedLocation;
        PKIX_PL_String *locationString = NULL;
        PKIX_UInt32 len = 0;
        char *locationAscii = NULL;
        char *startPos = NULL;
        char *endPos = NULL;

        PKIX_ENTER(LDAPREQUEST, "pkix_pl_LdapRequest_ParseLocation");
        PKIX_NULLCHECK_TWO(request, location);

        PKIX_TOSTRING(location, &locationString, plContext,
            "pkix_pl_GeneralName_ToString failed");

        PKIX_CHECK(PKIX_PL_String_GetEncoded
		   (locationString,
                   PKIX_ESCASCII,
                   (void **)&locationAscii,
                   &len,
                   plContext),
                   "PKIX_PL_String_GetEncoded failed");

        PKIX_PL_NSSCALLRV(LDAPREQUEST, parsedLocation, PORT_ArenaZAlloc,
                (request->arena, sizeof(struct pkix_pl_LdapLocation)));

        /* Skip "ldap:" */
        endPos = locationAscii;
        while (*endPos != ':' && *endPos != '\0') {
                endPos++;
        }
        if (*endPos == '\0') {
                PKIX_ERROR("Unexpected LDAP location string format: ldap:");
        }

        /* Skip "//" */
        endPos++;
        if (*endPos != '\0' && *(endPos+1) != '0' &&
            *endPos == '/' && *(endPos+1) == '/') {
                endPos += 2;
        } else {
                PKIX_ERROR("Unexpected LDAP location string format: //");
        }

        /* Get the server-site */
        startPos = endPos;
        while(*endPos != '/' && *(endPos) != '\0') {
                endPos++;
        }
        if (*endPos == '\0') {
                PKIX_ERROR
                    ("Unexpected LDAP location string format: <server-site>/");
        }

        len = endPos - startPos;
        endPos++;

        PKIX_PL_NSSCALLRV
                (LDAPREQUEST,
                parsedLocation->serverSite,
                PORT_ArenaZAlloc,
                (request->arena, len+1));

        PKIX_PL_NSSCALL(LDAPREQUEST, PORT_Memcpy,
                (parsedLocation->serverSite, startPos, len));

        *((char *)parsedLocation->serverSite+len+1) = '\0';

        /* Get filter strings in pointer array */
        startPos = endPos;
        PKIX_CHECK(pkix_pl_LdapRequest_ParseTokens
            (request,
            &startPos,
            (char ***) &parsedLocation->filterString,
            ',',
            '?',
            plContext),
            "pkix_pl_ldaprequest_ParseTokens failed");

        /* Get Attr Bits in pointer array */
        PKIX_CHECK(pkix_pl_LdapRequest_ParseTokens
            (request,
            (char **) &startPos,
            (char ***) &parsedLocation->attrBitString,
            ',',
            '\0',
            plContext),
            "pkix_pl_ldaprequest_ParseTokens failed");

        request->parsedLocation = parsedLocation;

cleanup:

        PKIX_PL_Free(locationAscii, plContext);
        PKIX_DECREF(locationString);

        PKIX_RETURN(LDAPREQUEST);
}
