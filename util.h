#ifndef UTIL_H
#define UTIL_H

#define ang2au 1.889725989;

#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include "atom.h"
#include "molecule.h"
#include <iomanip>
#include <omp.h>
using namespace std;

/* Search strings */
string geom_str=" Output coordinates";
string basislabel_str="    Basis function labels";
string overlap_str = " global array: AO ovl";
string nbasis_str="          This is a Direct SCF calculation.";
string tddft_str="  Convergence criterion met";
string tddft_state_stop_str="-----------------------------";
string tddft_task_str1="tddft";
string tddft_task_str2="TDDFT";
string tddft_task_str3="Tddft";
string mo_analysis_str="                       DFT Final Molecular Orbital Analysis";
string lindep_str = " !! The overlap matrix has";
string aobas_str = "          AO basis - number of functions:";

/* Number of columns for printing of matrices */
int ncol = 6;

/********************************************************
 * Functions
 * *******************************************************/

/**** Skip several lines in input file ****/
void getnlines(ifstream &in, char *temp, int n, int length) {
  for (int i=0; i<n; i++) {
    in.getline(temp,length);
  }
}

bool calcdip(Molecule *mol) {
  
  double dx = 0.;
  double dy = 0.;
  double dz = 0.;
  
  for (int i=0; i<mol->natoms; i++) {
    dx += mol->atoms[i].x * mol->atoms[i].tq;//indo;
    dy += mol->atoms[i].y * mol->atoms[i].tq;//indo;
    dz += mol->atoms[i].z * mol->atoms[i].tq;//indo;
 // cout<<mol->atoms[i].x<<" "<<mol->atoms[i].tq<<endl;
  }

  dx *= ang2au;
  dy *= ang2au;
  dz *= ang2au;

  mol->dipx = dx;
  mol->dipy = dy;
  mol->dipz = dz;
  
  cout<<"Transition dipole moment: "<<dx<<" x, "<<dy<<" y, "<<dz<<" z"<<endl;
  cout<<"Magnitude = "<<sqrt(dx*dx+dy*dy+dz*dz)/0.39345<<endl;
  return true;
}

/*** Calculate the transition charges for a given root ***/
bool calctqindo(Molecule *mol, int root) {
 double sum = 0.; 
  for (int atom=0; atom<mol->natoms; atom++) {

    mol->atoms[atom].tqindo = 0.;

    for (int b=0; b<mol->nbasis; b++) { //basis functions
      if (mol->nbasisatom[b] != atom+1) continue;
      for (int i=0; i<mol->nocc; i++) { //MO's ci's
        for (int j=mol->nocc; j<mol->nmo; j++) { //MO ci's
              mol->atoms[atom].tqindo += mol->ci[root*mol->nmo*mol->nmo+i+j*mol->nmo]
                  * mol->mos[i+b*mol->nmo] * mol->mos[j+b*mol->nmo];
        }//end unoccupied MO
      }//end occupied MO
    } //end NAO on atom of interest
    cout<<"atom "<<atom<<endl;
    mol->atoms[atom].tqindo *= sqrt(2);
    cout<<mol->nbasisatom[atom]<<" "<<mol->atoms[atom].type<<" "<<mol->atoms[atom].tqindo<<" "<<endl;
    sum += mol->atoms[atom].tqindo;
  } //end atoms
  
  cout<<"Net charge: "<<sum<<endl;
  return true;
}



/*** Calculate the transition charges for a given root ***/
bool calctq(Molecule *mol, int root) {
 double sum = 0.; 
  for (int atom=0; atom<mol->natoms; atom++) {
    double s2,s3,s4;
    s2=0.;
    s3=0.;
    s4=0.;
    mol->atoms[atom].tq = 0.;
    mol->atoms[atom].tqa = 0.;
    mol->atoms[atom].tqb = 0.;

    for (int b=0; b<mol->nbasis; b++) {
      if (mol->nbasisatom[b] != atom+1) continue;
      
#pragma omp parallel for reduction (+:s2,s3,s4)
      for (int c=0; c<mol->nbasis; c++) {
        //if (mol->nbasisatom[c] == atom+1) continue;
        
        for (int i=0; i<mol->nocc; i++) {
          for (int j=mol->nocc; j<mol->nbasis; j++) {
              /*mol->atoms[atom].tq*/s2 += mol->ci[i+j*mol->nmo+root*mol->nmo*mol->nmo]
                    * mol->mos[i+b*mol->nmo] * mol->mos[j+c*mol->nmo]
                    * mol->overlapm[c+b*mol->nbasis];
              //if (mol->os) {
                /*mol->atoms[atom].tqa*/s3 += mol->cia[i+j*mol->nmo+root*mol->nmo*mol->nmo]
                    * mol->mos[i+b*mol->nmo] * mol->mos[j+c*mol->nmo]
                    * mol->overlapm[c+b*mol->nbasis];
                /*mol->atoms[atom].tqb*/s4 += mol->cib[i+j*mol->nmo+root*mol->nmo*mol->nmo]
                    * mol->mos[i+b*mol->nmo] * mol->mos[j+c*mol->nmo]
                    * mol->overlapm[c+b*mol->nbasis];
              //}
          }//end unoccupied MO
        }//end occupied MO
      }//end all other AOs
    
    } //end NAO on atom of interest
    if (mol->os) {
      mol->atoms[atom].tqa = s3;
      mol->atoms[atom].tqb = s4;
      mol->atoms[atom].tq = s3+s4;
    } else
      mol->atoms[atom].tq = s2;

    /** Check on the sqrt(2), and if it's needed for open shell **/
    mol->atoms[atom].tq *= sqrt(2);
    mol->atoms[atom].tqa *= sqrt(2);
    mol->atoms[atom].tqb *= sqrt(2);
  cout<<atom<<" "<<mol->atoms[atom].type<<" "<<mol->atoms[atom].tq<<" "<<mol->atoms[atom].tqa<<" "<<mol->atoms[atom].tqb<<endl;
sum += mol->atoms[atom].tq;
  } //end atoms
  
  cout<<"Net charge: "<<sum<<endl;
  return true;
}

/*** Read in MO vectors from movec file ***/
/* str             - input, file name to read
 * mol            - input, Molecule object
 * mol->occupation - output, occupation numbers
 * mol->moeigenv   - output, MO eigenvalues
 * mol->mos        - output, MO in orthonormal basis
 */

bool readMOs(string str, Molecule *mol) {
  cout.precision(10);
  char tempc[1000];
  ifstream infile;
  infile.open(str.c_str());

  if (!infile.is_open()) {
    cout<<"Error opening MO file"<<endl;
    return false;
  } else {
    cout<<"Opening file: "<<str<<endl;
  }
  
  /* Skip right to the occupation numbers */
  getnlines(infile,tempc,14,500);
  mol->nocc = 0;
  mol->nuocc = 0;
  for (int i=0; i<mol->nmo; i++) {
    infile>>tempc;
    mol->occupation[i] = (int)atof(tempc);
    if (mol->occupation[i] == 0) {
      mol->nuocc++;
    } else {
      mol->nocc++;
    }
  }
cout<<"HOMO = "<<mol->nocc<<", # virt. orb. = "<<mol->nuocc<<endl;
  /* Read in eigenvalues */
  for (int i=0; i<mol->nmo; i++) {
    infile>>tempc;
    mol->moeigenv[i] = atof(tempc);
  }

  /* Read in MO vectors */
  for (int i=0; i<mol->nmo; i++)
    for (int j=0; j<mol->nbasis; j++) {
      infile>>tempc;
      mol->mos[i+j*mol->nmo] = atof(tempc);
    }
  return true;
}



/*** Function to parse an NWChem output file ***/
/* Returns true if successful, false otherwise */
/* 
 * str        - input, log file name
 * mol       - input, Molecule object
 *
 * mol->nroots    -output, number of tddft roots
 * mol->natoms    -output, number of atoms in molecule
 * mol->atom     -output, individual atom objects
 * mol->atom.type  - output, element
 * mol->atom.x,y,z - output, geometry
 * mol->atom.charge  output, charge on atom
 */

bool parseLog(string str, Molecule *mol) {
    
    ifstream infile;
    infile.precision(9);
    infile.open(str.c_str());
    cout<<"Opening file: "<<str.c_str()<<endl;

    char tempc[1000];
    int natoms=0;
    int nbasis=0;
    bool tddftstack = true;

    while(infile.getline(tempc, 1000)) {
      string temps(tempc);
      int current = infile.tellg();
      
/** Get Number of Roots in TDDFT **/
      if ((temps.compare(0,5,tddft_task_str1,0,5) == 0 ||
        temps.compare(0,5,tddft_task_str2,0,5) == 0 ||
        temps.compare(0,5,tddft_task_str3,0,5) == 0) &&
        tddftstack) {
        cout<<"getting nroots"<<endl;
        while (temps.compare(0,3,"end",0,3) !=0 ) {
          if (temps.compare(0,6,"nroots")==0 ||
            temps.compare(0,6,"NROOTS")==0 ||
            temps.compare(0,6,"Nroots")==0 ||
            temps.compare(0,6,"NRoots")==0) {
          
            temps = strtok(tempc," ");
            temps = strtok(NULL," ");
            mol->setnroots(atoi(temps.c_str()));
            tddftstack = false;
          } //end nroots
          infile>>ws;
          infile.getline(tempc,1000);
          temps = tempc;
        }
      } //end tddft


/** Geometry **/     
      if (temps.compare(0,10,geom_str,0,10) == 0) {
      cout<<"getting geometry"<<endl;
        /* Skip a few lines to get to the geometry */
        getnlines(infile,tempc,3,1000);

        /* Read geometry */
        //get position in file
/*        int pos = infile.tellg();
        while (temps != "Atomic") {
          infile>>ws;
          infile.getline(tempc,1000);
          temps = strtok(tempc," ");
          //get natoms
          if (temps != "Atomic") {
            natoms++;
          }
        }

        /* Make an molecule entry and declare natoms for the molecule */
//        mol->allocateMemAtoms(natoms);

        //return to top of geometry
//        infile.seekg(pos);
        infile.getline(tempc,1000);
        temps = strtok(tempc," ");
        int i=0;
        while(temps!= "Atomic") {
          //Assign atom numbers
          mol->atoms[i].num = atoi(temps.c_str());
          temps = strtok(NULL," ");

          //Get atom type
          mol->atoms[i].type = temps;

          //Get atom charge
          temps = strtok(NULL," ");
          mol->atoms[i].charge = atof(temps.c_str());

          //X-coordinate
          temps = strtok(NULL," ");
          mol->atoms[i].x = atof(temps.c_str());
          
          //Y-coordinate
          temps = strtok(NULL," ");
          mol->atoms[i].y = atof(temps.c_str());

          //Z-coordinate
          temps = strtok(NULL," ");
          mol->atoms[i].z = atof(temps.c_str());

          infile>>ws;
          infile.getline(tempc,1000);
          temps = strtok(tempc," ");
          i++;
        }
      } //end geometry

/** Number of AO basis functions **/
      
      if (temps.compare(0,40,nbasis_str,0,40)==0) {
        cout<<"getting nAO"<<endl;
        infile.getline(tempc,1000);
        temps = strtok(tempc,":");
        temps = strtok(NULL,":");

        //allocate memory for molecule's properties
        mol->allocateMem(atoi(temps.c_str()));
      } //end ao basis

/** Number linearly dependent vectors **/
    if (temps.compare(0,25,lindep_str,0,25)==0) {
      cout<<"getting nlindep vectors"<<endl;
      temps = strtok(tempc," ");
      for (int i=0; i<5; i++) temps = strtok(NULL," ");
      mol->allocateMemLindep(mol->nbasis,atoi(temps.c_str()));
    }


/** Basis Function Labels **/
      
      if (temps.compare(0,20,basislabel_str,0,20) == 0) {
      cout<<"getting basis function labels"<<endl;
        getnlines(infile,tempc,4,1000);
        temps = tempc;
        
        for (int i=0; i<mol->natoms; i++) 
          mol->atoms[i].nao = 0;

        while (temps.compare(0,10,overlap_str,1,10) != 0) {
          temps = tempc;
          temps = strtok(tempc," ");

          //get function number
          int num = atoi(temps.c_str());

          //get atom number
          temps = strtok(NULL," ");
          mol->nbasisatom[num-1] = atoi(temps.c_str());

          //increment number of AOs on atom
          mol->atoms[atoi(temps.c_str())-1].nao ++;
          
          //get atom element
          temps = strtok(NULL," ");
          mol->nbasisatomelements[num-1] = temps;

          //get type of AO
          temps = strtok(NULL," ");
          mol->nbasisatomorbitals[num-1] = temps;
          
          infile>>ws;
          infile.getline(tempc,1000);
          temps = tempc;
        } //end import basis functions

      } //end basis label

/** Basis function overlap matrix **/
      if (temps.compare(0,20,overlap_str,1,20) == 0) {
        cout<<"getting overlap matrix"<<endl;
        int nblocks = mol->nbasis/ncol;
        for (int block=0; block<nblocks; block++) {
          if (block == 0)
            getnlines(infile,tempc,4,1000);
          else
            getnlines(infile,tempc,3,1000);
          for (int i=0; i<mol->nbasis; i++) {
            temps = strtok(tempc," ");
            for (int j=0; j<ncol; j++) {
              temps = strtok(NULL," ");
              mol->overlapm[i+(j+ncol*block)*mol->nbasis]
                = atof(temps.c_str());
                //if (i==189 && j==5) cout<<mol->overlapm[i+(j+ncol*block)*mol->nbasis]<<endl;
            } // end col
              infile.getline(tempc,1000);
          }  // end rows
        } //end full blocks
        int remain = mol->nbasis%ncol;
        cout<<"remain = "<<remain<<endl;
        if (remain > 0) {
          getnlines(infile,tempc,3,1000);
        for (int i=0; i<mol->nbasis; i++) {
          temps = strtok(tempc," ");
          for (int j=0; j<remain; j++) {
            temps = strtok(NULL," ");
            mol->overlapm[i+(j+nblocks*ncol)*mol->nbasis] 
              = atof(temps.c_str());
            if (i==189 && j==remain-1) cout<<mol->overlapm[i+(j+nblocks*ncol)*mol->nbasis]<<endl;
          } //end col
          infile.getline(tempc,1000);
        } //end row
        } //end remain
      } //end overlap matrix

/** Final MO analysis **/
    if (temps.compare(0,40,mo_analysis_str,0,40) == 0) {
      cout<<"getting MO's, nmo= "<<mol->nmo<<endl;
      int current;
      infile.getline(tempc,1000);
      infile>>ws;
      infile.getline(tempc,1000);
      temps = tempc;

      for (int i=0; i<mol->nmo; i++) {

        temps = strtok(tempc," ");
        temps = strtok(NULL," ");

        //which MO
        int nmo = atoi(temps.c_str())-1;

        temps = strtok(NULL,"=");
        temps = strtok(NULL,"=");
        
        //occupation number
        mol->occupation[nmo] = (int)atof(temps.c_str());

        getnlines(infile,tempc,3,1000);
        
        bool first=true;
        //vectors
        while (temps != "Vector" && temps != "center"
            && temps != "---------------------------------------------") {

          if (first) {
            infile>>temps;
            first = false;
          }
          int nbas = atoi(temps.c_str())-1;
          infile>>temps;
          mol->mos[nmo+nbas*mol->nmo] = atof(temps.c_str());
          current = infile.tellg();
          for (int j=0; j<3; j++) infile>>temps;
          if (temps == "d") {
            infile>>temps;
          }
          infile>>temps;
          if (temps != "Vector" && temps != "center" 
            && temps != "---------------------------------------------") { 
              nbas = atoi(temps.c_str())-1;
              infile>>temps;
              mol->mos[nmo+nbas*mol->nmo] = atof(temps.c_str());
              //if (nmo==0) cout<<nbas<<" "<<mol->mos[nmo+nbas*mol->nmo]<<" "<<temps<<endl;
              for (int j=0; j<3; j++) infile>>temps;
              if (temps == "d") {
                infile>>temps;
              }
              infile>>temps;
            }
      }//end vector coeffs

      infile.seekg(current);
      infile.getline(tempc,1000);
      infile.getline(tempc,1000);
      infile.getline(tempc,1000);
      }
    }//end MOs


/** TDDFT excited states **/
    if (temps.compare(0,10,tddft_str,0,10) == 0) {
      bool openshell = false;
      int dum = 1;
      cout<<"getting CI vectors"<<endl;
      mol->allocateMemTddft();
      
      getnlines(infile,tempc,3,1000);
      temps = tempc;
      if (temps.compare(0,5,"  <S2>",0,5) == 0) {
        getnlines(infile,tempc,2,1000);
        openshell = true;
        mol->os = true;
      } else
        infile.getline(tempc,1000);

      for (int root=0; root<mol->nroots; root++) {

        infile.getline(tempc,1000);
        temps = strtok(tempc," ");
        
        if (openshell) 
          dum = 3;
        else
          dum = 4;

        for (int i=0; i<dum; i++) temps = strtok(NULL," ");
        mol->excenergy[root] = atof(temps.c_str());
        if (openshell) {
          infile>>ws;
          infile.getline(tempc,1000);
          temps = strtok(tempc," ");
          temps = strtok(NULL," ");
          temps = strtok(NULL," ");
          mol->spin[root] = atof(temps.c_str());
        }
        if (openshell) 
          dum = 2;
        else
          dum = 2;

        getnlines(infile,tempc,dum,1000);
      
        for (int k=0; k<3; k++) {
          temps = strtok(tempc," ");
          temps = strtok(NULL," ");
      
          for (int j=0; j<3; j++) {
            for (int i=0; i<2; i++) temps = strtok(NULL," ");
              mol->transmoment[k+j*3+root*9] = atof(temps.c_str());
          }
          infile.getline(tempc,1000);
        }//end trans moment

        //get oscillator strength
        temps = strtok(tempc," ");
        for (int i=0; i<3; i++) temps=strtok(NULL," ");
        mol->oscstrength[root] = atof(temps.c_str());
        infile.getline(tempc,1000);
      
        infile.getline(tempc,1000);
        temps = tempc;
        
        //get CI coeffs
        //nrpa is for RPA
        /** Currently this is confirmed to work with 
         * closed shell tddft, and open shell cis
         **/
        int nrpa = 1;     
        double ycoeff;
        double sumci = 0.;
        double sumci2 = 0.;
        bool spina;
        while (temps.compare(0,10,tddft_state_stop_str,0,10) != 0 &&
              temps.compare(0,10,"Target root",0,10) !=0 ) {
          temps = strtok(tempc," ");
          temps = strtok(NULL," ");
          int row = atoi(temps.c_str())-1;
          if (openshell) 
            dum = 5;
          else
            dum = 4;

          for (int i=0; i<dum; i++) temps = strtok(NULL," .");
          int col = atoi(temps.c_str())-1;
          if (openshell) 
            dum = 1;
          else
            dum = 2;
          for (int i=0; i<dum; i++) temps = strtok(NULL," ");
          
          if (openshell) {
            if (temps.compare(0,4,"alpha",0,4)==0) {
              spina = true; //alpha
            } else {
              spina = false; //beta
            }
            //cout<<temps<<" "<<spina<<endl;
            for (int i=0; i<2; i++) temps = strtok(NULL," ");
          }
          //if (nrpa%2 == 0 || mol->excMethod == "cis" || mol->excMethod == "CIS") {
            mol->ci[row + col*mol->nmo + root*mol->nmo*mol->nmo] += atof(temps.c_str());
            
            /** Alpha/Beta CI coefficients for unrestricted **/
            if (openshell && spina) {
              mol->cia[row + col*mol->nmo + root*mol->nmo*mol->nmo] += atof(temps.c_str());
           //   cout<<"alpha "<<temps<<endl;
            } else if (openshell && (!spina)) {
              mol->cib[row + col*mol->nmo + root*mol->nmo*mol->nmo] += atof(temps.c_str());
             // cout<<"beta "<<temps<<endl;
            }

            sumci += atof(temps.c_str())*atof(temps.c_str());
            //cout<<row<<" "<<col<<" "<<temps<<" "<<atof(temps.c_str())<<" temps "<<sumci<<endl;
          //}
          //CTC check this and make compatible with CIS 9-12-14
          /*if (nrpa%2 == 1 ) {
            mol->ci[row + col*mol->nmo + root*mol->nmo*mol->nmo] += atof(temps.c_str());
            sumci2 += atof(temps.c_str())*atof(temps.c_str());
          }
         */ nrpa++;
          infile>>ws;
          infile.getline(tempc,1000);
          temps=tempc;
        } //end CI coeffs
        //cout<<"sum ci "<<sumci<<" "<<sumci2<<endl;
//        for (int i=0; i<mol->nmo; i++)
//          for (int j=0; j<mol->nmo; j++)
//            mol->ci[i+j*mol->nmo+root*mol->nmo*mol->nmo] /= (sumci-sumci2);
      } //end nroots
    }//end tddft
  } //end reading logfile
    return true;
}

/** Print some stuff **/
void printStuff(Molecule *mol,int nroot) {
  ofstream outfile;
  outfile.precision(16);
  outfile.open("aodata.dat");
  
  /* print atomic numbers, elements, positions, and tq's */
  /* natoms first */
  outfile<<"Natoms : "<<mol->natoms<<endl;
  for (int i=0; i<mol->natoms; i++) {
    outfile<<i+1<<" "<<mol->atoms[i].type.c_str()<<" "
      <<mol->atoms[i].x<<" "<<mol->atoms[i].y<<" "<<mol->atoms[i].z<<" "
      <<mol->atoms[i].tq<<" "
      <<mol->atoms[i].tqa<<" "
      <<mol->atoms[i].tqb<<endl;
  }
}


#endif // UTIL_H
