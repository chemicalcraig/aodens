#ifndef MOLECULE_H
#define MOLECULE_H

#include <vector>
#include "atom.h"
using namespace std;

class Molecule {
  public:
  int nao,nbasis;
  int nmo;
  int nroots;
  int nocc,nuocc;
  double dipx,dipy,dipz;
  int nlindep;
  string excMethod;
  double *activeCharges;

  double *ci; //ci coefficients
  //vector<int> nbasisatom; //number of basis functions on an atom
  int* nbasisatom; //number of basis functions on an atom
  //vector<string> nbasisatomorbital; //ao name on atom
  string *nbasisatomorbitals;
  //vector<string> nbasisatomelement; //which element basis function belongs to
  string *nbasisatomelements; //which element basis function belongs to
  //vector<double> overlapm; //overlap matrix, S
  double *overlapm;
  //vector<double> excenergy; //tddft exc. energy
  double *excenergy;
  //vector<double> transmoment; //tddft transition moments
  double *transmoment;
  //vector<double> oscstrength; //tddft osc strength
  double *oscstrength;
  //vector<double*> cicoeffs; //tddft ci coefficients
  //vector<int> occupation; //occupation numbers, no fractional
  int * occupation;
  //vector<double> moeigenv; //mo eigenvalues
  double *moeigenv;
  //vector<double> mos; //mo vectors
  double *mos;

  double *posx,*posy,*posz;
  Atom *atoms;
  //vector<Atom> atomsv;
  int natoms;
  int nx,ny,nz;
  double dx1,dx2,dx3,dy1,dy2,dy3,dz1,dz2,dz3;
  double ox,oy,oz;
  double ***dens, ***densrad;

  void setnatoms(int n) {natoms = n;};
  void setxyz(int x, int y, int z) {nx=x;ny=y;nz=z;};

  Molecule();
  ~Molecule() {};

  /* Memory allocation routines */
  void allocateMem(const int nb);
  void allocateMemLindep(const int nb, const int nlindep);
  void allocateMemAtoms(const int na);
  void allocateMemTddft();
  void setnroots(const int n) {this->nroots=n;};
};

#endif // MOLECULE_H
