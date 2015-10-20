/*
 * TinyEKF - Extended Kalman Filter in a small amount of code
 *
 * Based on Extended_KF.m by Chong You
 * https://sites.google.com/site/chongyou1987/
 *
 * Syntax:
 * [Xo,Po] = Extended_KF(f,g,Q,R,Z,Xi,Pi)
 *
 * State Equation:
 * X(n+1) = f(X(n)) + w(n)
 * where the state X has the dimension N-by-1
 * Observation Equation:
 * Z(n) = g(X(n)) + v(n)
 * where the observation y has the dimension M-by-1
 * w ~ N(0,Q) is gaussian noise with covariance Q
 * v ~ N(0,R) is gaussian noise with covariance R
 * Input:
 * f: function for state transition, it takes a state variable Xn and
 * returns 1) f(Xn) and 2) Jacobian of f at Xn. As a fake example:
 * function [Val, Jacob] = f(X)
 * Val = sin(X) + 3;
 * Jacob = cos(X);
 * end
 * g: function for measurement, it takes the state variable Xn and
 * returns 1) g(Xn) and 2) Jacobian of g at Xn.
 * Q: process noise covariance matrix, N-by-N
 * R: measurement noise covariance matrix, M-by-M
 *
 * Z: current measurement, M-by-1
 *
 * Xi: "a priori" state estimate, N-by-1
 * Pi: "a priori" estimated state covariance, N-by-N
 * Output:
 * Xo: "a posteriori" state estimate, N-by-1
 * Po: "a posteriori" estimated state covariance, N-by-N
 *
 * Algorithm for Extended Kalman Filter:
 * Linearize input functions f and g to get fy(state transition matrix)
 * and H(observation matrix) for an ordinary Kalman Filter:
 * State Equation:
 * X(n+1) = fy * X(n) + w(n)
 * Observation Equation:
 * Z(n) = H * X(n) + v(n)
 *
 * 1. Xp = f(Xi)                     : One step projection, also provides
 * linearization point
 *
 * 2.
 * d f    |
 * fy = -----------|                 : Linearize state equation, fy is the
 * d X    |X=Xp                        Jacobian of the process model
 *
 *
 * 3.
 * d g    |
 * H  = -----------|                 : Linearize observation equation, H is
 * d X    |X=Xp                        the Jacobian of the measurement model
 *
 *
 * 4. Pp = fy * Pi * fy' + Q         : Covariance of Xp
 *
 * 5. K = Pp * H' * inv(H * Pp * H' + R): Kalman Gain
 *
 * 6. Xo = Xp + K * (Z - g(Xp))      : Output state
 *
 * 7. Po = [I - K * H] * Pp          : Covariance of Xo
 */

#include "linalg.hpp"

class TinyEKF {

protected:

    virtual void f(double * X, double * Xp, double ** fy) = 0;
    
    virtual void g(double * Xp, double * gXp, double ** H) = 0;    

    TinyEKF(int n, int m)
    {
        this->n = n;
        this->m = m;
        
        this->_X = newvec(n);

        this->_P = newnewmat(n, n);
        this->_Q = newnewmat(n, n);
        this->_R = newnewmat(m, m);
        this->_G = newnewmat(n, m);
        
        this->_H   = newnewmat(m, n);
        this->_fy  = newnewmat(n, n);

        //this->_X   = newvec(n);
        this->_Xp  = newvec(n);
        this->_gXp = newvec(m);

        this->_Pp   = newnewmat(n, n);
        
        this->_Ht   = newnewmat(n, m);
        this->_fyt  = newnewmat(n, n);

        this->_eye_n_n = newnewmat(n, n);
        eye(this->_eye_n_n, 1);

        this->_tmp_m    = newvec(m);
        this->_tmp_n    = newvec(n);
        this->_tmp_m_m  = newnewmat(m, m);
        this->_tmp2_m_m  = newnewmat(m, m);
        this->_tmp_m_n  = newnewmat(m, n);
        this->_tmp_n_m  = newnewmat(n, m);
        this->_tmp2_n_m = newnewmat(n, m);
        this->_tmp_n_n  = newnewmat(n, n);
     }
    
   ~TinyEKF()
    {
        /*
        deletemat(this->P, this->n);
        deletemat(this->Q, this->n);
        deletemat(this->R, this->m);
        deletemat(this->G, this->n);
        deletemat(this->H, this->m);

        deletemat(this->fy, n);
        deletemat(this->fyt, this->n);        
        
        delete this->X;
        delete this->Xp;
        delete this->gXp;
        deletemat(this->Pp,  this->n);
        deletemat(this->Ht, this->n);

        deletemat(this->eye_n_n, this->n);
        
        deletemat(this->tmp_n_n, this->n);
        deletemat(this->tmp_m_n,  this->m);

        deletemat(this->tmp_n_m, this->n);        
        deletemat(this->tmp2_n_m,  this->m);
        deletemat(this->tmp_m_m,  this->m);
        delete this->tmp_n;
        delete this->tmp_m;
        */
    }

   void setP(int i, int j, double value)
   {
       this->_P->data[i][j] = value;
   }
  
   void setQ(int i, int j, double value)
   {
       this->_Q->data[i][j] = value;
   }

   void setR(int i, int j, double value)
   {
       this->_R->data[i][j] = value;
   }

   void setX(int i, double value)
   {
       this->_X->data[i] = value;
   }

 private:

    int n;          // state values
    int m;          // measurement values

    vec_t * _X;
    mat_t * _P;
    mat_t * _Q;
    mat_t * _R;

    mat_t  * _G;    // Kalman gain; a.k.a. K
    
    vec_t  * _Xp;   // output of state-transition function
    mat_t  * _fy;   // Jacobean of process model
    mat_t  * _H;    // Jacobean of measurement model
    vec_t  * _gXp;
    
    mat_t  * _Ht;
    mat_t  * _fyt;
    mat_t  * _Pp;

    mat_t  * _eye_n_n;

    // temporary storage
    mat_t  * _tmp_n_m;
    mat_t  * _tmp_n_n;
    mat_t  * _tmp_m_n;
    vec_t  * _tmp_m;
    vec_t  * _tmp_n;
    mat_t  * _tmp2_n_m;
    mat_t  * _tmp_m_m;
    mat_t  * _tmp2_m_m;

public:

    void update(double * Z)
    {        
        // 1, 2
        this->f(this->_X->data, this->_Xp->data, this->_fy->data);

        // 3
        this->g(this->_Xp->data, this->_gXp->data, this->_H->data);     

        // 4
        mul(this->_fy, this->_P, this->_tmp_n_n);
        transpose(this->_fy, this->_fyt);
        mul(this->_tmp_n_n, this->_fyt, this->_Pp);
        add(this->_Pp, this->_Q);

        // 5
        transpose(this->_H, this->_Ht);
        mul(this->_Pp, this->_Ht, this->_tmp_n_m);
        mul(this->_H, this->_Pp, this->_tmp_m_n);
        mul(this->_tmp_m_n, this->_Ht, this->_tmp2_m_m);
        add(this->_tmp2_m_m, this->_R);
        invert(this->_tmp2_m_m, this->_tmp_m_m);
        mul(this->_tmp_n_m, this->_tmp_m_m, this->_G);

        // 6
        this->_tmp_m->data = Z;
        sub(this->_tmp_m, this->_gXp);
        mul(this->_G, this->_tmp_m, this->_X);

        // 7
        mul(this->_G, this->_H, this->_tmp_n_n);
        negate(this->_tmp_n_n);
        add(this->_tmp_n_n, this->_eye_n_n);
        mul(this->_tmp_n_n, this->_Pp, this->_P);

        dump(this->_P, "%+10.4f"); exit(0);
     }
};
