#ifdef SUPPORT_RTF

#include "rtftype.h"
#include "rtfdecl.h"

int cGroup;
char fSkipDestIfUnk;
long cbBin;
long lParam;
RDS rds;
RIS ris;

CHP chp;
PAP pap;
SEP sep;
DOP dop;
SAVE *psave;
FILE *fpIn;

// %%Function: ecPushRtfState
//
// Save relevant info on a linked list of SAVE structures.

int ecPushRtfState(void)
{
    SAVE *psaveNew = malloc(sizeof(SAVE));
    if (!psaveNew)
        return ecStackOverflow;

    psaveNew -> pNext = psave;
    psaveNew -> chp = chp;
    psaveNew -> pap = pap;
    psaveNew -> sep = sep;
    psaveNew -> dop = dop;
    psaveNew -> rds = rds;
    psaveNew -> ris = ris;
    ris = risNorm;
    psave = psaveNew;
    cGroup++;
    return ecOK;
}

// %%Function: ecPopRtfState
//
// If we're ending a destination (that is, the destination is changing),
// call ecEndGroupAction.
// Always restore relevant info from the top of the SAVE list.

int ecPopRtfState(void)
{
    SAVE *psaveOld;
    int ec;

    if (!psave)
        return ecStackUnderflow;

    if (rds != psave->rds)
    {
        if ((ec = ecEndGroupAction(rds)) != ecOK)
            return ec;
    }
    chp = psave->chp;
    pap = psave->pap;
    sep = psave->sep;
    dop = psave->dop;
    rds = psave->rds;
    ris = psave->ris;

    psaveOld = psave;
    psave = psave->pNext;
    cGroup--;
    free(psaveOld);
    return ecOK;
}

// %%Function: ecParseRtfKeyword
//
// Step 2:
// get a control word (and its associated value) and
// call ecTranslateKeyword to dispatch the control.

int ecParseRtfKeyword(FILE *fp)
{
    int ch;
    char fParam = fFalse;
    char fNeg = fFalse;
    int param = 0;
    char *pch;
    char szKeyword[30];
    char *pKeywordMax = &szKeyword[30];
    char szParameter[20];
    char *pParamMax = &szParameter[20];

    lParam = 0;
    szKeyword[0] = '\0';
    szParameter[0] = '\0';
    if ((ch = getc(fp)) == EOF)
        return ecEndOfFile;
    if (!isalpha(ch))           // a control symbol; no delimiter.
    {
        szKeyword[0] = (char) ch;
        szKeyword[1] = '\0';
        return ecTranslateKeyword(szKeyword, 0, fParam);
    }
    for (pch = szKeyword; pch < pKeywordMax && isalpha(ch); ch = getc(fp))
        *pch++ = (char) ch;
    if (pch >= pKeywordMax)
        return ecInvalidKeyword;	// Keyword too long
    *pch = '\0';
    if (ch == '-')
    {
        fNeg  = fTrue;
        if ((ch = getc(fp)) == EOF)
            return ecEndOfFile;
    }
    if (isdigit(ch))
    {
        fParam = fTrue;         // a digit after the control means we have a parameter
        for (pch = szParameter; pch < pParamMax && isdigit(ch); ch = getc(fp))
            *pch++ = (char) ch;
        if (pch >= pParamMax)
            return ecInvalidParam;	// Parameter too long
        *pch = '\0';
        param = atoi(szParameter);
        if (fNeg)
            param = -param;
        lParam = param;
    }
    if (ch != ' ')
        ungetc(ch, fp);
    return ecTranslateKeyword(szKeyword, param, fParam);
}

// %%Function: ecParseChar
//
// Route the character to the appropriate destination stream.

int ecParseChar(int ch)
{
    if (ris == risBin && --cbBin <= 0)
        ris = risNorm;
    switch (rds)
    {
    case rdsSkip:
        // Toss this character.
        return ecOK;
    case rdsNorm:
        // Output a character. Properties are valid at this point.
        return ecPrintChar(ch);
    default:
    // handle other destinations....
        return ecOK;
    }
}

// %%Function: ecCheckValidChar
//
// Return ecOK if valid character, or 1.

int ecCheckValidChar(int ch)
{
    if (ris == risBin && --cbBin <= 0)
        ris = risNorm;
    switch (rds)
    {
    case rdsSkip:
        // Toss this character.
        return -1;
    case rdsNorm:
        // Output a character. Properties are valid at this point.
        return ecOK;
    default:
    // handle other destinations....
        return -1;
    }
}

//
// %%Function: ecPrintChar
//
// Send a character to the output file.

int ecPrintChar(int ch)
{
    // unfortunately, we do not do a whole lot here as far as layout goes...
//    putchar(ch);
    return ecOK;
}
#endif