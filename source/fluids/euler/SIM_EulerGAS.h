/*******************************************************************************
    VFXEPOCH - Physically based simulation VFX

    Copyright (c) 2016 Snow Tsui <trevor.miscellaneous@gmail.com>

    All rights reserved. Use of this source code is governed by
    the MIT license as written in the LICENSE file.
*******************************************************************************/
#ifndef _SIM_EULER_GAS_H_
#define _SIM_EULER_GAS_H_

#include "fluids/euler/SIM_FluidBase.h"
#include "utl/PCGSolver/util.h"
#include "utl/PCGSolver/sparse_matrix.h"
#include "utl/PCGSolver/blas_wrapper.h"
#include "utl/PCGSolver/pcg_solver.h"

using namespace VFXEpoch;
using namespace VFXEpoch::Solvers;

namespace VFXEpoch{
  namespace Solvers{

    class EulerGAS2D;
    class EulerGAS3D;

    class EulerGAS2D : public Euler_Fluid2D_Base{
    /***************************** User Parameters *****************************/
    public:
      struct Parameters{
      public:
        Parameters(){
          dimension.m_x = 0; dimension.m_y = 0; dt = 0.0; 
          space.m_x = space.m_y = 0.0;
          h = 0.0;
          dt = 0.0;
          buoyancy_alpha = buoyancy_beta = 0.0;
          tolerance = 0.0;
          max_iterations = 0;
          num_particles = 0;
          density_source = 0.0;
        }
        Parameters(Vector2Di _dimension, double _h, Vector2Dd _space, double _dt, 
                   double _buoyancy_alpha, double _buoyancy_beta, double _tolerance,
                   double _diff, double _visc, int _max_iterations, int _num_particles, 
                   double _density_source): 
                   dimension(_dimension), h(_h), space(_space), dt(_dt), 
                   buoyancy_alpha(_buoyancy_alpha), buoyancy_beta(_buoyancy_beta), 
                   tolerance(_tolerance), diff(_diff), visc(_visc), max_iterations(_max_iterations), 
                   num_particles(_num_particles), density_source(_density_source){}
        Parameters(const Parameters& src){
          dimension = src.dimension;
          h = src.h;
          space = src.space;
          dt = src.dt;
          buoyancy_alpha = src.buoyancy_alpha; buoyancy_beta = src.buoyancy_beta;
          tolerance = src.tolerance;
          visc = src.visc;
          diff = src.diff;
          max_iterations = src.max_iterations;
          num_particles = src.num_particles;
          density_source = src.density_source;
        }
        Parameters& operator=(const Parameters& rhs){
          dimension = rhs.dimension;
          h = rhs.h;
          space = rhs.space;
          dt = rhs.dt;
          buoyancy_alpha = rhs.buoyancy_alpha; buoyancy_beta = rhs.buoyancy_beta;
          tolerance = rhs.tolerance;
          diff = rhs.diff;
          visc = rhs.visc;
          max_iterations = rhs.max_iterations;
          num_particles = rhs.num_particles;
          density_source = rhs.density_source;
          return *this;
        }
        ~Parameters(){ clear(); }
      public:
        inline void clear(){
          dimension.m_x = dimension.m_y = 0;
          space.m_x = space.m_y = 0.0;
          h = 0.0;
          dt = 0.0;
          buoyancy_alpha = buoyancy_beta = 0.0;
          tolerance = 0.0;
          diff = 0.0;
          visc = 0.0;
          max_iterations = 0;
          num_particles = 0;
          density_source = 0.0;
        }

        friend inline ostream&
        operator<<(ostream& os, const Parameters& params) {
          os << std::setprecision(4) << setiosflags(ios::fixed);
          os << "Dimension = " << params.dimension.m_x << " x " << params.dimension.m_y << endl;
          os << "Increment h = " << params.h << endl;
          os << "Dx = " << params.space.m_x << ", Dy = " << params.space.m_y << endl;
          os << "Diffuse rate = " << params.diff << endl;
          os << "Viscousity = " << params.visc << endl;
          os << "Time step = " << params.dt << endl;
          os << "Buoyancy alpha & beta = "
             << params.buoyancy_alpha << ", "
             << params.buoyancy_beta << endl;
          os << "Vorticity Confinement Epsilon = " << params.vort_conf_eps << endl;
          os << "Number of particles = " << params.num_particles << endl;
          os << "Density source = " << params.density_source << endl;
          os << "Iterations in solver = " << params.max_iterations << endl;
          os << "External force strength = " << params.external_force_strength << endl;
          return os;
        }
      public:
        Vector2Di dimension;
        Vector2Dd space;
        double h;
        double dt;
        double buoyancy_alpha, buoyancy_beta;
        double vort_conf_eps;
        double tolerance;
        double density_source;
        double external_force_strength;
        double diff;
        double visc;
        int max_iterations;
        int num_particles;
      };
    /***************************** User Parameters END *************************/

    public:
      EulerGAS2D();
      EulerGAS2D(Parameters _user_params);
      EulerGAS2D(const EulerGAS2D& src);
      EulerGAS2D& operator=(const EulerGAS2D& rhs);
      ~EulerGAS2D();
    public:
      // TODO: Implement overload functions
      bool init(Parameters params); // Overload
      void step(); // Overload
      void close(); // Overload
      void add_source(int i, int j); // Overload
      void add_external_force(VFXEpoch::VECTOR_COMPONENTS component, int i, int j);
      void add_particles(VFXEpoch::Particle2D p);
    public:
      void set_inside_boundary(Grid2DCellTypes boundaries);
      void set_domain_boundary(VFXEpoch::BOUNDARY boundary_type, VFXEpoch::EDGES_2DSIM edge);
    public:
      void set_user_params(Parameters params);
      EulerGAS2D::Parameters get_user_params();
    protected:
      void set_domain_boundary_wrapper(Grid2DfScalarField& field);
      void density_diffuse(Grid2DfScalarField& dest, Grid2DfScalarField ref);
      void dynamic_diffuse(Grid2DfScalarField& dest, Grid2DfScalarField ref);
      void advect_vel();
      void advect_curl();
      void advect_den();
      void advect_tmp();
      void advect_particles();
      void project();
    protected:
      void apply_buoyancy();
      void pressure_solve(); // Overload
      void apply_gradients();
      void extrapolate(Grid2DfScalarField& grid, const Grid2DfScalarField& weights, 
                       Grid2DCellTypes& mask, Grid2DCellTypes& mask0);
      void get_grid_weights();
      void clamp_vel();
      void setup_pressure_coef_matrix();
      Vector2Df trace_rk2(const Vector2Df& pos, float dt);
      Vector2Df get_vel(const Vector2Df& pos);
      float get_den(const Vector2Df& pos);
      float get_curl(const Vector2Df& pos);
      float get_tmp(const Vector2Df& pos);
    private:
    /*********************** Pressure Solver Parameters ************************/
      struct PressureSolverParams{
        PCGSolver<double> pcg_solver;
        SparseMatrixd sparse_matrix;
        vector<double> rhs;
        vector<double> pressure;
        
        inline void clear(){
          pcg_solver.clear();
          sparse_matrix.clear();
          rhs.clear();
          pressure.clear();
        }
      };
    /*********************** Pressure Solver Parameters END ********************/
    private:
      Grid2DfScalarField u, u0;
      Grid2DfScalarField v, v0;
      Grid2DfScalarField uw, vw;
      Grid2DfScalarField d, d0;
      Grid2DfScalarField t, t0;
      Grid2DfScalarField omega, omega0;
      Grid2DfScalarField nodal_solid_phi;
      Grid2DCellTypes inside_mask, inside_mask0;
      BndConditionPerEdge domain_boundaries[4];
      vector<VFXEpoch::Particle2D> particles_container;
      Parameters user_params;
      PressureSolverParams pressure_solver_params;
    };
  }
}

#endif
