#ifndef ODESOLVERS_HPP
#define ODESOLVERS_HPP

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>


/** \file ODESolvers.hpp
 *  \brief Solve ordinary differential equations.
 *   
 *  Library to solve ordinary differential equations.
 *  The library uses polymorphism to design a model (class model)
 *  from building blocks.
 *  Those building blocks are the vector field (class vectorField)
 *  and the numerical scheme (class numericalScheme).
 */


/*
 * Class declarations:
 */

// /** \brief Data structure containing the simulation parameters.
//  *    
//  * Data structure containing the simulation parameters.
//  */
// typedef struct {
//   double length;    //!< Duration of the simulation.
//   double spinup;    //!< Spinup period length to be removed.
//   size_t sampling;  //!< Time step between each recorded state
// } simulationParam;


/** \brief Abstract class defining a vector field.
 *
 *  Abstract class defining a vector field. 
 */
class vectorField {
  const size_t dim;   //!< Phase space dimension
public:
  /** \brief Default constructor. */
  vectorField() {}
  /** \brief Constructor setting the dimension. */
  vectorField(const size_t dim_) : dim(dim_) {}
  /** \brief Destructor. */
  ~vectorfield() {}
  /** \brief Dimension access method. */
  size_t getDim() { return dim; }
  /** \brief Virtual method for evaluating the vector field at a given state. */
  virtual void evalField(gsl_vector *state, gsl_vector *field) = 0;
};


/** \brief Vector field defined by a linear operator.
 *
 *  Vector field defined by a linear operator:
 *  \f$ F(x) = A x \f$.
 */
class linearField : vectorField {
  gsl_matrix *A;  //!< Matrix representation of the linear operator \f$ A \f$.
public:
  /** \brief Default constructor. */
  linearField(){}
  /** \brief Construction by copying the matrix of the linear operator. */
  linearField(gsl_matrix *A_) : vectorField(A_->size1)
  { gsl_matrix_memcpy(A, A_); }
  /** Destructor freeing the matrix. */
  ~linearField(){ gsl_matrix_free(A); }

  /** \brief Evaluate the linear vector field at a given state. */
  void evalField(gsl_vector *state, gsl_vector *field);
};


/** \brief Vector field for the normal form of the cusp bifurcation.
 *
 *  Vector field for the normal form of the cusp bifurcation
 * (Guckenheimer and Holmes, 1988, Strogatz 1994):
 *  \f$ F(x) = h + r x - x^3 \f$.
 */
class cuspField : vectorField {
  double r;  //< Parameter \f$ r \f$ related to the pitchfork bifurcation.
  double h;  //< Parameter \f$ h \f$ related to the catastrophe.
  
public:
  /** \brief Default constructor, just defining the phase space dimension. */
  cuspField() : vectorField(1) {}
  /** \brief Constructor defining the model parameters. */
  cuspField(const double r_, const double h_)
    : vectorField(1), r(r_), h(h_) {}
  ~cuspField(){}

  /** \brief Evaluate the cusp vector field at a given state. */
  void evalField(gsl_vector *state, gsl_vector *field);
};


/** \brief Vector field for the Lorenz 63 model.
 *
 *  Vector field for the Lorenz 63 model (Lorenz, 1963):
 *
 *  \f$ F_1(x) = \sigma (x_2 - x_1) \f$
 *
 *  \f$ F_2(x) = x_1 (\rho - x_3) - x_2 \f$
 *
 *  \f$ F_3(x) = x_1 x_2 - \beta x_3 \f$.
 */
class Lorenz63 : vectorField {
  const double rho;     //!< Parameter \f$ \rho \f$ corresponding to the Rayleigh number
  const double sigma;   //!< Parameter \f$ \sigma \f$
  const double beta;    //!< Parameter \f$ \beta \f$
  
public:
  /** \brief Default constructor, just defining the phase space dimension. */
  Lorenz63() : dim(3) {}
  /** \brief Constructor defining the model parameters. */
  Lorenz63(const double rho_, const double sigma_, const double beta_)
    : vectorField(3),  rho(rho_), sigma(sigma_), beta(beta_) {}
  /** \brief Destructor. */
  ~Lorenz63() {}

  /** \brief Evaluate the vector field of the Lorenz 63 model for a given state. */
  void evalField(gsl_vector *state, gsl_vector *field);

};


/** \brief Abstract defining a numerical scheme.
 *
 *  Abstract class defining a numerical scheme used to integrate the model.
 */
class numericalScheme {
  const size_t dim;      //!< Dimension of the phase space
  const size_t dimWork;  //!< Dimension of the workspace used to evaluate the field
  
protected:
  const double dt;       //!< Time step of integration.
  gsl_matrix *work;      //!< Workspace used to evaluate the vector field

public:
  /** \brief Default constructor. */
  numericalScheme() {}
  
  /** \brief Constructor defining the dimensions, time step and allocating workspace. */
  numericalScheme(const size_t dim_, const size_t dimWork_, const double dt_)
    : dim(dim_), dimWork(dimWork_), dt(dt_)
  { work = gsl_matrix_alloc(dimWork, dim); }
  
  /** \brief Destructor freeing workspace. */
  ~numericalScheme() { gsl_matrix_free(work); }

  /** \brief Virtual method to integrate the model one step forward. */
  virtual void stepForward(const vectorField *field, gsl_vector *currentState) = 0;
};


/** \brief Euler scheme for numerical integration. 
 * 
 *  Euler scheme for numerical integration. 
 */
class Euler : public numericalScheme {
public:
  /** \brief Default constructor */
  Euler(){}
  
  /** \brief Constructor defining integration parameters and allocating workspace. */
  Euler(const size_t dim_, const double dt_)
    : numericalScheme(dim_, 1, dt_){}
  
  /** \brief Destructor. */
  ~Euler() {}

  /** \brief One time-step Euler forward Integration of the model. */
  void stepForward(const vectorField *field, gsl_vector *currentState);
};


/** \brief Runge-Kutta 4 scheme for numerical integration. 
 * 
 *  Runge-Kutta 4 scheme for numerical integration. 
 */
class RungeKutta4 : numericalScheme {
public:
  /** \brief Default constructor */
  RungeKutta4(){}

  /** \brief Constructor defining integration parameters and allocating workspace. */
  RungeKutta4(const vectorField *field, const double dt_)
    : numericalScheme(field->getDim(), 5, dt_){}
  ~RungeKutta4() {}
  
  /** \brief One time-step Runge-Kutta 4 forward Integration of the model. */
  void stepForward(const vectorField *field, gsl_vector *currentState);
};


/** \brief Numerical model class.
 *
 *  Numerical model class.
 *  A model is defined by a vector field and a numerical scheme.
 *  The current state of the model is also recorded.
 */
class model {
  const size_t dim;              //!< Phase space dimension
  const vectorField *field;      //!< Vector field
  const numericalScheme *scheme; //!< Numerical scheme
  gsl_vector *currentState;      //!< Current state
  
public:
  /** \brief Default constructor. */
  model() {}
  
  /** \brief Constructor assigning a vector field and a numerical scheme
   *  and setting initial state to origin. */
  model(vectorField *field_, numericalScheme *scheme_)
    : dim(field_->getDim()), field(field_), scheme(scheme_)
  { currentState = gsl_vector_calloc(dim); }

  /** \brief Constructor assigning a vector field and a numerical scheme
   *  and set initial state. */
  model(vectorField *field_, numericalScheme *scheme_, gsl_vector *initState)
    : dim(field_->getDim()), field(field_), scheme(scheme_)
  {
    currentState = gsl_vector_alloc(dim);
    gsl_vector_memcpy(currentState, initState);
  }

  /** \brief Destructor freeing memory. */
  ~model() { gsl_vector_free(currentState); }

  /** \brief One time-step forward Integration of the model using the numerical scheme. */
  void stepForward();

  /** \brief Integrate the model forward for a given period. */
  gsl_matrix *model::integrateForward(const double length, const double spinup,
			       const size_t sampling);
};


/*
 * Method definitions:
 */

/**
 ** Vector fields definitions:
 **/

/** 
 * Evaluate the linear vector field at a given state.
 * \param[in]  state State at which to evaluate the vector field.
 * \param[out] field Vector resulting from the evaluation of the vector field.
 */
void
linearField::evalField(gsl_vector *state, gsl_vector *field)
{
  // Linear field: apply operator A to state
  gsl_blas_dgemv(CblasNoTrans, 1., A, state, 0., field);

  return;
}


/** 
 * Evaluate the vector field of the normal form of the cusp bifurcation
 * (Guckenheimer & Holmes, 1988, Strogatz,  1994) at a given state:
 * 
 *         F(x) = h + r x - x^3
 *
 * \param[in]  state State at which to evaluate the vector field.
 * \param[out] field Vector resulting from the evaluation of the vector field.
 */
void
cuspField::evalField(gsl_vector *state, gsl_vector *field)
{
  // F(x) = h + r x - x^3
  gsl_vector_set(field, 0, h + r * gsl_vector_get(state, 0) 
		 - pow(gsl_vector_get(state, 0), 3));
  
  return;
}


/** 
 * Evaluate the vector field of the Lorenz 63 model
 * (Lorenz, 1963) at a given state.
 *
 *  \f$ F_1(x) = \sigma (x_2 - x_1) \f$
 *
 *  \f$ F_2(x) = x_1 (\rho - x_3) - x_2 \f$
 *
 *  \f$ F_3(x) = x_1 x_2 - \beta x_3 \f$
 * \param[in]  state State at which to evaluate the vector field.
 * \param[out] field Vector resulting from the evaluation of the vector field.
 */
void
Lorenz63::evalField(gsl_vector *state, gsl_vector *field)
{

  // Fx = sigma * (y - x)
  gsl_vector_set(field, 0, param->sigma
		 * (gsl_vector_get(state, 1) - gsl_vector_get(state, 0)));
  // Fy = x * (rho - z) - y
  gsl_vector_set(field, 1, gsl_vector_get(state, 0)
		 * (param->rho - gsl_vector_get(state, 2))
		 - gsl_vector_get(state, 1));
  // Fz = x*y - beta*z
  gsl_vector_set(field, 2, gsl_vector_get(state, 0) * gsl_vector_get(state, 1)
		 - param->beta * gsl_vector_get(state, 2));
 
  return;
}


/**
 ** Numerical schemes definitions:
 **/

/**
 * Integrate one step forward for a given vector field and state
 * using the Euler scheme.
 * \param[in]     field Vector field to evaluate.
 * \param[in/out] Current state to update by one time step.
 */
void
Euler::stepForward(const vectorField *field, gsl_vector *currentState)
{
  gsl_vector_view tmp = gsl_matrix_row(work, 0); 

  // Get vector field
  field->evalField(currentState, &tmp.vector)
  
  // Scale by time step
  gsl_vector_scale(&tmp.vector, dt);

  // Add previous state
  gsl_vector_add(currentState, &tmp.vector);

  return;
}


/**
 * Integrate one step forward for a given vector field and state
 * using the Runge-Kutta 4 scheme.
 * \param[in]     field Vector field to evaluate.
 * \param[in/out] Current state to update by one time step.
 */
void
RungeKutta4::stepForward(const vectorField *field, gsl_vector *currentState)
{
  /** Use views on a working matrix not to allocate memory
   *  at each time step */
  gsl_vector_view k1, k2, k3, k4, tmp; 

  // Assign views
  tmp = gsl_matrix_row(work, 0);
  k1 = gsl_matrix_row(work, 1);
  k2 = gsl_matrix_row(work, 2);
  k3 = gsl_matrix_row(work, 3);
  k4 = gsl_matrix_row(work, 4);
  
  // First increament
  field->evalField(currentState, &k1.vector)
  gsl_vector_scale(&k1.vector, dt);
  
  gsl_vector_memcpy(&tmp.vector, &k1.vector);
  gsl_vector_scale(&tmp.vector, 0.5);
  gsl_vector_add(&tmp.vector, currentState);

  // Second increment
  lorenzField(&tmp.vector, &k2.vector);
  gsl_vector_scale(&k2.vector, dt);
  
  gsl_vector_memcpy(&tmp.vector, &k2.vector);
  gsl_vector_scale(&tmp.vector, 0.5);
  gsl_vector_add(&tmp.vector, currentState);

  // Third increment
  lorenzField(&tmp.vector, &k3.vector);
  gsl_vector_scale(&k3.vector, dt);
  
  gsl_vector_memcpy(&tmp.vector, &k3.vector);
  gsl_vector_add(&tmp.vector, currentState);

  // Fourth increment
  lorenzField(&tmp.vector, &k4.vector);
  gsl_vector_scale(&k4.vector, dt);

  gsl_vector_scale(&k2.vector, 2);
  gsl_vector_scale(&k3.vector, 2);
  gsl_vector_memcpy(&tmp.vector, &k1.vector);
  gsl_vector_add(&tmp.vector, &k2.vector);
  gsl_vector_add(&tmp.vector, &k3.vector);
  gsl_vector_add(&tmp.vector, &k4.vector);
  gsl_vector_scale(&tmp.vector, 1. / 6);

  // Update state
  gsl_vector_add(currentState, &tmp.vector);

  return;
}


/**
 ** Model definitions:
 **/

/**
 * Integrate one step forward the model by calling the numerical scheme.
 */
void
model::stepForward()
{
  // Apply numerical scheme to step forward
  scheme.stepForward(field, currentState);
    
  return;
}


/**
 * Integrate the model forward for a given period.
 * \param[in]  length   Duration of the integration.
 * \param[in]  Spinup   Initial integration period to remove.
 * \param[in]  sampling Time step at which to save states.
 * \return              Matrix to record the states.
 */
gsl_matrix *
model::integrateForward(const double length, const double spinup,
			const size_t sampling)
{
  size_t nt = length / scheme->getTimeStep();
  size_t ntSpinup = spinup / simParam->getTimeStep();
  gsl_matrix *data = gsl_matrix_alloc((size_t) (nt / sampling), dim);

  // Get spinup
  for (size_t i = 1; i <= ntSpinup; i++)
    stepForward();
  
  // Get record
  for (size_t i = ntSpinup+1; i <= nt; i++)
    {
      stepForward();

      // Save state
      if (i%sampling == 0)
	gsl_matrix_set_row(data, (i - ntSpinup) / sampling - 1, currentState);
    }

  return data;
}

#endif
