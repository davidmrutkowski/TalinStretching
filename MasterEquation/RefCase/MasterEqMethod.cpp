/**
 * Copyright (C) 2022 Lehigh University.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 * Author: David Rutkowski (dmr518@lehigh.edu)
 */
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <boost/math/tools/roots.hpp>
#include <chrono>

#define PI 3.14159265

double F_bond(double bond_k, double l_zero, double curr_bond_length)
{
    return bond_k*(curr_bond_length - l_zero);
}
    

double P_event(double k_unfold, double alpha, double bond_k, double l_zero, double curr_bond_length, double dt)
{
    double curr_force = fabs(F_bond(bond_k, l_zero, curr_bond_length));
    double Rdt = k_unfold * exp(alpha * curr_force) * dt;
    
    /*if(Rdt > 1.0)
        Rdt = 1.0;
        
    return Rdt;*/
    
    return 1.0 - exp(-Rdt);
}

double kj_value(double k_unfold, double alpha, double bond_k, double l_zero, double curr_bond_length, double dt)
{
    double curr_force = fabs(F_bond(bond_k, l_zero, curr_bond_length));
    double kj_value = k_unfold * exp(alpha * curr_force);
    
    /*if(Rdt > 1.0)
        Rdt = 1.0;
        
    return Rdt;*/
    
    return kj_value;
}

double Langevin(double x)
{
    if(x == 0)
        return 0.0;
    else
        return cosh(x)/sinh(x) - 1.0/x;
}
        
double invLangevin(double x)
{
    double x2 = x*x;
    double x4 = x2*x2;
    double x6 = x2*x4;
    double x8 = x4*x4;
    
    double val = x*(3.0 - 1.00651*x2 - 0.962251*x4 + 1.4353*x6 - 0.48953*x8) / (1.0-x) / (1.0+1.01524*x);
    
    return val;
}

int factorial(int n)
{
    std::cout << "factorial doesnt work for large values of n" << std::endl;
    exit(0);
    
    if(n == 0)
    {
        return 1;
    }
    else if(n > 0)
    {
        int result = 1;
        for(int i = n; i > 1; i--)
        {
            result *= i;
        }
        
        return result;
    }
    else
    {
        exit(0);
        return -1;
    }
}

//https://stackoverflow.com/questions/9330915/number-of-combinations-n-choose-r-in-c/53983114
int nCr(int n,int k)
{   
    //int tempResult = factorial(n) / (factorial(k) * factorial(n-k));
    //std::cout << "nCr " << n << " " << k << " : " << tempResult << std::endl;
    
    //return tempResult;
    
    if (k > n) return 0;
    if (k * 2 > n) k = n-k;
    if (k == 0) return 1;

    int result = n;
    for( int i = 2; i <= k; ++i ) {
        result *= (n-i+1);
        result /= i;
    }
    
    
    return result;
    
}

double pow_integer(double x, int pow)
{
    if(pow == 0)
    {
        return 1.0;
    }
    else if(pow > 0)
    {
        
        double result = x;
        for(int i = 1; i < pow; i++)
        {
            result *= x;
        }
        
        return result;
    }
    else
    {
        return -1.0;
    }
}

double root_func(double x, double aminoAcidB, int talinN, double kT, double l_zero, double l_t, double bond_k1, int N_bonds, int uf)
{
    double temp_val = Langevin(aminoAcidB/kT * bond_k1 * (x - l_zero)) - (l_t - x*(N_bonds - (double)uf)) / (aminoAcidB * talinN) / ((double)uf);
    return temp_val;
}
    
int main(int argc, char** argv)
{
    double k_unbind = 0.17;
    double alpha_unbind = 0.125;

    double k_unfold = 2.5E-5;
    double alpha_unfold = 4.1/4.114;
    
    int N_bonds = 12+2;
    
    int n_unfoldable_domains = N_bonds-2;    
    
    if(argc >= 2)
    {
        for(int i = 1; i < argc; i++)
        {
            switch(i)
            {
                case 1:
                    n_unfoldable_domains = atoi(argv[1]);
                    std::cout << "n_unfoldable_domains: " << n_unfoldable_domains << std::endl;
                    break;
                default:
                    std::cout << "Ignoring additional arguments beyond 2nd" << std::endl;
            }
        }
    }

    double bond_k1 = 10000.0;
    double bond_k2 = 1000.0;

    double v_retro = 0.02;

    double l_zero = 0.002;
    double l_zero_two = l_zero;

    double dt = 0.0001;
    

    int step = 1;

    double kT = 1.38E-23*1e12*1E6*300.0;
    int talinN = 145;
    double aminoAcidB = 0.38 / 1000.0;
    
    
    std::vector <double> unfolded_probabilities_prior_alt(n_unfoldable_domains + 1);
    for(int i = 1; i < unfolded_probabilities_prior_alt.size(); i++)
    {
        unfolded_probabilities_prior_alt[i] = 0.0;
    }
    unfolded_probabilities_prior_alt[0] = 1.0;

    std::vector <double> unfolded_probabilities_curr_alt(n_unfoldable_domains + 1);
    for(int i = 0; i < unfolded_probabilities_curr_alt.size(); i++)
    {
        unfolded_probabilities_curr_alt[i] = 0.0;
    }
    
    
    std::vector <double> unfolded_integrin_bond_length(n_unfoldable_domains);
    for(int i = 0; i < unfolded_integrin_bond_length.size(); i++)
    {
        unfolded_integrin_bond_length[i] = 0.0;
    }
    
    
    std::vector <double> base_prob_unfold(n_unfoldable_domains);
    for(int i = 0; i < base_prob_unfold.size(); i++)
    {
        base_prob_unfold[i] = 0.0;
    }
    
    std::vector <double> base_prob_unbind(n_unfoldable_domains+1);
    for(int i = 0; i < base_prob_unbind.size(); i++)
    {
        base_prob_unbind[i] = 0.0;
    }
    
    std::vector <double> base_kj_unbind(n_unfoldable_domains+1);
    for(int i = 0; i < base_kj_unbind.size(); i++)
    {
        base_kj_unbind[i] = 0.0;
    }
    
    std::vector <double> prob_unbind_first_time(n_unfoldable_domains+1);
    for(int i = 0; i < prob_unbind_first_time.size(); i++)
    {
        prob_unbind_first_time[i] = 0.0;
    }
    
    std::vector <double> time_list;

    //double curr_integrin_work = 0.0;
    double curr_integrin_work_alt = 0.0;

    std::ofstream output_file;
    std::stringstream ss;
    ss<<k_unfold;
    std::string k_unfold_string = ss.str();
    
    output_file.open ("n_unfoldable_" + std::to_string(n_unfoldable_domains) + ".csv");

    output_file << "Time[s],prob_unbound,alt_integrin_force[pN],alt_work_done[pN]";
    output_file << "\n";
    
    
    double tmp_output_prob_unfold = 0.0;
    double tmp_output_integrin_force = 0.0;
    
    double l_t_0 = (N_bonds)*l_zero;
    
    int output_step_frequency = 100;
    
    clock_t t1,t2;
	t1=clock();
    
    while(step*dt*v_retro*1000 < 500)
    {
        //double l_t = sqrt(l_zero*l_zero*N_bonds*N_bonds + v_retro*v_retro* (dt*dt*step*step));
        //double l_t = l_zero*N_bonds + v_retro*dt*step;
        double l_t = sqrt(pow_integer(l_t_0*sin(PI*0.25), 2) + pow_integer(l_t_0*cos(PI*0.25) + v_retro*dt*step, 2));
        
        double equal_bond_length = l_t / (double)N_bonds;
        
        // prob single bond unfolds when none are unfolded
        
        base_prob_unbind[0] = P_event(k_unbind, alpha_unbind, bond_k1, l_zero, equal_bond_length, dt);
        base_kj_unbind[0] = kj_value(k_unbind, alpha_unbind, bond_k1, l_zero, equal_bond_length, dt);
        
        if(n_unfoldable_domains >= 1)
        {
            base_prob_unfold[0] = P_event(k_unfold, alpha_unfold, bond_k1, l_zero, equal_bond_length, dt);
        }
        
        for(int uf = 1; uf < n_unfoldable_domains+1; uf++)
        {    
            // unfold spring bond, 1
            //unfolded_integrin_bond_length[uf-1] = - (-uf*bond_k1*l_zero + uf*bond_k2*l_zero_two - bond_k2*l_t) / (uf*bond_k1 - uf*bond_k2 + bond_k2*N_bonds)
            
            // unfold entropic bond, 2
            //func = lambda x : Langevin(aminoAcidB/kT * bond_k1 * (x - l_zero)) - (l_t - x*(N_bonds - uf)) / (aminoAcidB * talinN) / float(uf)
            unfolded_integrin_bond_length[uf-1] = equal_bond_length*0.8;
            
            auto x = boost::math::tools::bisect([aminoAcidB, talinN, kT, l_zero, l_t, bond_k1, N_bonds, uf](double x){return root_func(x, aminoAcidB, talinN, kT, l_zero, l_t, bond_k1, N_bonds, uf); },
                    0.0,
                    l_t,
                    [](double x,double y){return abs(x-y) < 1E-16;}
            );
            
            unfolded_integrin_bond_length[uf-1] = 0.5 * (x.first + x.second);

            base_prob_unbind[uf] = P_event(k_unbind, alpha_unbind, bond_k1, l_zero, unfolded_integrin_bond_length[uf-1], dt);
            base_kj_unbind[uf] = kj_value(k_unbind, alpha_unbind, bond_k1, l_zero, unfolded_integrin_bond_length[uf-1], dt);
            
            if(uf <= n_unfoldable_domains - 1)
            {
                // prob single bond unfolds when uf bonds are unfolded
                base_prob_unfold[uf] = P_event(k_unfold, alpha_unfold, bond_k1, l_zero, unfolded_integrin_bond_length[uf-1], dt);
                //std::cout << uf << " " << unfolded_integrin_bond_length[uf-1] << " " << base_prob_unbind[uf] << " " << base_prob_unfold[uf] << std::endl;
            }
        }
        
        
        // reset unfolded probabilities list        
        for(int i = 0; i < n_unfoldable_domains+1; i++)
        {
            unfolded_probabilities_curr_alt[i] = 0.0;
        }
        
        // i is the number of domains that are unfolded at time tn-1, j is the number of domains that are unfolded after this timestep at tn
        for(int i = 0; i < n_unfoldable_domains+1; i++)
        {
            for(int j = i; j < n_unfoldable_domains+1; j++)
            {
                int num_unfold = j - i;
                
                if(n_unfoldable_domains == 0)
                {
                    unfolded_probabilities_curr_alt[j] = unfolded_probabilities_prior_alt[i];
                }
                else
                {
                    //new state probability = old state probability * number of ways transition can occur * transition probability
                    
                    double tmp_val = unfolded_probabilities_prior_alt[i] * nCr(n_unfoldable_domains-i, num_unfold) * pow_integer(1.0 - base_prob_unfold[i], n_unfoldable_domains-j) * pow_integer(base_prob_unfold[i], num_unfold);
                    
                    unfolded_probabilities_curr_alt[j] += tmp_val;
                }
            }
            
            unfolded_probabilities_curr_alt[i] -= base_prob_unbind[i]*unfolded_probabilities_prior_alt[i];
        }
        
        
        double tmpForceAltProbSum = 0.0;
        for(int i = 0; i < n_unfoldable_domains; i++)
        {
            tmpForceAltProbSum += unfolded_probabilities_curr_alt[i]*(base_kj_unbind[i]);
        }
        
        
        
        //double curr_cos_angle = cos(atan(l_t_0 / (v_retro * step*dt)));
        double curr_cos_angle = cos(atan((l_t_0*sin(PI*0.25)) / (l_t_0*cos(PI*0.25) + v_retro*step*dt)));
        
        double curr_integrin_force_alt = unfolded_probabilities_curr_alt[0] * F_bond(bond_k1, l_zero, equal_bond_length)*(base_kj_unbind[0]);
        
        // equation 14
        curr_integrin_work_alt += F_bond(bond_k1, l_zero, equal_bond_length)*curr_cos_angle*dt*v_retro*1000*unfolded_probabilities_curr_alt[0];
        
        for(int i = 1; i < n_unfoldable_domains+1; i++)
        {
            curr_integrin_force_alt += unfolded_probabilities_curr_alt[i] * F_bond(bond_k1, l_zero, unfolded_integrin_bond_length[i-1])*(base_kj_unbind[i]);
            
            curr_integrin_work_alt += F_bond(bond_k1, l_zero, unfolded_integrin_bond_length[i-1])*curr_cos_angle*dt*v_retro*1000*unfolded_probabilities_curr_alt[i];
        }
        
        
        if(tmpForceAltProbSum <= 0.1)
        {
            curr_integrin_force_alt = F_bond(bond_k1, l_zero, unfolded_integrin_bond_length[n_unfoldable_domains-1]);
        }
        else
        {
        
            curr_integrin_force_alt = curr_integrin_force_alt / tmpForceAltProbSum;
        }
        
   
        if(step % output_step_frequency == 0)
        {
                    
            time_list.push_back(step*dt);
            
            
            if(tmp_output_prob_unfold < 1E-300)
            {
                tmp_output_prob_unfold = 0.0;
            }
            
            
            output_file << time_list[time_list.size()-1] << ",";
            
            // alternate formulation of prob unbind
            double prob_bound = 0.0;
            for(int i = 0; i < unfolded_probabilities_curr_alt.size(); i++)
            {
                prob_bound += unfolded_probabilities_curr_alt[i];
            }
            output_file << 1.0 - prob_bound << ",";
            
            
            output_file << curr_integrin_force_alt << "," << curr_integrin_work_alt;
            
            /*for(int i = 0; i < n_unfoldable_domains+1; i++)
            {
                output_file << unfolded_probabilities_curr_alt[i] << ",";
            }*/
            output_file << std::endl;
        }
        
        tmp_output_prob_unfold = 0.0;
        tmp_output_integrin_force = 0.0;
        
        for(int i = 0; i < unfolded_probabilities_curr_alt.size(); i++)
        {
            unfolded_probabilities_prior_alt[i] = unfolded_probabilities_curr_alt[i];
        }
        
        //tmp_output_prob_unfold += total_prob_unbind_this_step;
        //sum_prob_unbind += total_prob_unbind_this_step;
        step += 1;
    }
    
    t2=clock();
	float diff ((float)t2-(float)t1);
	float seconds = diff / CLOCKS_PER_SEC;

    output_file << "\n";
	output_file << "Total sim time: " << seconds << std::endl;

    //std::cout << sum_prob_unbind << std::endl;
}