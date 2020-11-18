/**CFile****************************************************************

  FileName    [ioWriteTlcd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    []

  Author      [Micael Popping]
  
  Affiliation [UFPel - Universidade Federal de Pelotas]

  Date        []

  Revision    []

  Teste

***********************************************************************/

#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Io_NtkWriteTlcdOne( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteTlcdHeader( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteTlcdCis( FILE * pFile, Abc_Ntk_t * pNtk );
static void Io_NtkWriteTlcdCos( FILE * pFile, Abc_Ntk_t * pNtk );
static int  Io_NtkWriteTlcdCheck( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteTlcd( Abc_Ntk_t * pNtk, char * pFileName )
{
    FILE * pFile;

    assert( Abc_NtkIsAigNetlist(pNtk) );
    if ( Abc_NtkLatchNum(pNtk) > 0 )
        printf( "Warning: only combinational portion is being written.\n" );

    // check that the names are fine for the TLCD format
    if ( !Io_NtkWriteTlcdCheck(pNtk) )
        return;

    // start the output stream
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteTlcd(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "# Equations for \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );

    // write the equations for the network
    Io_NtkWriteTlcdOne( pFile, pNtk );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Write one network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteTlcdOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Vec_Vec_t * vLevels;
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pFanin;
    word thruth;
    int t, pW[3], i, k;

    Io_NtkWriteTlcdHeader( pFile, pNtk);

    // write the PIs
    Io_NtkWriteTlcdCis( pFile, pNtk );

    // write the POs
    Io_NtkWriteTlcdCos( pFile, pNtk );

    // write each internal node
    vLevels = Vec_VecAlloc( 10 );
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkObjNumMax(pNtk) );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        fprintf( pFile, "%s", Abc_ObjName(Abc_ObjFanout0(pNode)) );
        // calculating the thresholds gates
        thruth = Hop_ManComputeTruth6((Hop_Man_t*) pNtk->pManFunc, (Hop_Obj_t*) pNode->pData, Abc_ObjFaninNum(pNode));
        t = Extra_ThreshHeuristic(&thruth, Abc_ObjFaninNum(pNode), pW);
        // inserting the weights and treshold
        for(int j = Abc_ObjFaninNum(pNode) - 1; j >= 0; j--)
            fprintf(pFile, " %d", pW[j]);
        fprintf(pFile, " %d", t);
        // set the input names
        Abc_ObjForEachFanin( pNode, pFanin, k )
            Hop_IthVar((Hop_Man_t *)pNtk->pManFunc, k)->pData = Abc_ObjName(pFanin);
        // write the formula
        Hop_ObjPrintTlcd( pFile, (Hop_Obj_t *)pNode->pData, vLevels, 0 );
        // end tlg
        fprintf(pFile, "\n");
    }
    Extra_ProgressBarStop( pProgress );
    Vec_VecFree( vLevels );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteTlcdHeader( FILE * pFile, Abc_Ntk_t * pNtk )
{
    int numInputs = Abc_NtkCiNum(pNtk);
    int numThresholdGates = Abc_NtkNodeNum(pNtk);
    int maxVariableIndex = numInputs + numThresholdGates;
    int numLatches = Abc_NtkLatchNum(pNtk);
    int numOutputs = Abc_NtkCoNum(pNtk);

    fprintf(pFile, "tlg %d %d %d %d %d\n", maxVariableIndex, numInputs, numLatches, numOutputs, numThresholdGates);
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteTlcdCis( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int i;

    Abc_NtkForEachCi( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanout0(pTerm);

        fprintf( pFile, "%s\n", Abc_ObjName(pNet) );
    }
}

/**Function*************************************************************

  Synopsis    [Writes the primary input list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_NtkWriteTlcdCos( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pTerm, * pNet;
    int i;

    Abc_NtkForEachCo( pNtk, pTerm, i )
    {
        pNet = Abc_ObjFanin0(pTerm);
        
        fprintf( pFile, "%s\n", Abc_ObjName(pNet) );
    }
}

/**Function*************************************************************

  Synopsis    [Make sure the network does not have offending names.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_NtkWriteTlcdCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    char * pName = NULL;
    int i, k, Length;
    int RetValue = 1;

    // make sure the network does not have proper names, such as "0" or "1" or containing parentheses
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        pName = Nm_ManFindNameById(pNtk->pManName, i);
        if ( pName == NULL )
            continue;
        Length = strlen(pName);
        if ( pName[0] == '0' || pName[0] == '1' )
        {
            RetValue = 0;
            break;
        }
        for ( k = 0; k < Length; k++ )
            if ( pName[k] == '(' || pName[k] == ')' || pName[k] == '!' || pName[k] == '*' || pName[k] == '+' )
            {
                RetValue = 0;
                break;
            }
        if ( k < Length )
            break;
    }
    if ( RetValue == 0 )
    {
        printf( "The network cannot be written in the EQN format because object %d has name \"%s\".\n", i, pName );
        printf( "Consider renaming the objects using command \"short_names\" and trying again.\n" );
    }
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

