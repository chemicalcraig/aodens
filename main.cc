/********************************************
 * Program to calculate atomic transition
 * densities using an AO expansion.
 *
 * CTC 7-31-2014
 *
 * For further information, see:
 * JPCB, 117, 2032, 2013
 * JPCC, 114, 20834, 2010
 * *******************************************/

#include "main.h"
#include <vector>
using namespace std;

int main(int argc, char **argv) {
  
  //open input file
  comfile.open(argv[1]);

  //parse input file
  char tempc[1000];
  string temps;

  /* 
   * Get calculation type
   * "transden" - get ao transition densities
   * "overlap"  - get overlap of two chromophore
   *              transition densities
   */
  
  comfile.getline(tempc,1000);
  temps = strtok(tempc, ":");
  temps = strtok(NULL, " :");
  
  if (temps == "transden") {
    
    /* Allocate memory for molecule object */
    Molecule molecule;
    
    /* get natoms */
    comfile.getline(tempc,1000);
    temps = strtok(tempc,":");
    temps = strtok(NULL," :");
        
    /* Make a molecule entry and declare natoms for the molecules */
    molecule.allocateMemAtoms(atoi(temps.c_str()));

    /* get nwchem log file name */
    comfile.getline(tempc,1000);
    temps = strtok(tempc,":");
    temps = strtok(NULL," :");
    string temps3 = temps;
    
    /* get MO vectors from movec file */
    /* File must be generated using the
     * mov2asc utility provided with NWChem
     */
    /* get movecs file name */
    comfile.getline(tempc,1000);
    temps = strtok(tempc," ");
    for (int i=0; i<3; i++) temps=strtok(NULL," ");
    string temps2 = temps;
    
    /* Excitation level of theory */
    comfile.getline(tempc,1000);
    temps = strtok(tempc, " :");
    temps = strtok(NULL, " ");
    molecule.excMethod = temps;
    cout<<temps<<endl;
     
    /* read in which root to consider */
    comfile.getline(tempc,1000);
    temps = strtok(tempc,": ");
    temps = strtok(NULL,": ");
    int nroot = atoi(temps.c_str())-1;   
    
    /* Parse Log file */
    parseLog(temps3,&molecule);

    /* read in mo's */
    readMOs(temps2,&molecule);
cout<<"Done reading overlap eigenvectors"<<endl;

    /* Calculate the transition charges, tq's 
     * for an excited state, given by nroot
     */

    /* Calculate tq's */
    calctq(&molecule,nroot);
    //calctqindo(&molecule,nroot);

    /* Calculate transition dipole moment */
    calcdip(&molecule);


  } else if (temps == "overlap") {

  } else {
    cout<<"Unrecognized calculation type!"<<endl;
    return -1;
  }

  return 0;
}
