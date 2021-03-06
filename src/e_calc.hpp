/* 	e_calc.hpp
	----------
	YM: 20.12.2020 - compute some values
	YM: 11.01.2021 - class S_VD, energy in double, instead float
	YM: 19.01.2021 - correct test of NB elements found
*/
#ifndef E_CALC_HPP
#define E_CALC_HPP

// Storage class of common variables, always in main module
#ifdef MAIN
  #define CLASS // intern...
#else
  #define CLASS extern
#endif

#include "e230.h"
#include "e230_05.hpp"


/* sample of buffer content

/LGZ4ZMR120AC.K76
F.F.0(00000000)
0.0.2(  978880)
C.1.0(30863358)
C.1.1(        )
1.8.1(000000.000*kWh)
1.8.2(004281.837*kWh)
1.8.0(004281.837*kWh)
2.8.1(000000.000*kWh)
2.8.2(014605.516*kWh)
2.8.0(014605.516*kWh)
32.7.0(238*V)
52.7.0(236*V)
72.7.0(237*V)
31.7.0(000.67*A)
51.7.0(000.86*A)
71.7.0(000.99*A)
36.7.0(000.09*kW)
56.7.0(000.11*kW)
76.7.0(000.07*kW)
33.7.0(0.81)
53.7.0(0.57)
73.7.0(0.32)
C.7.0(0010)
C.7.1(0029)
C.7.2(0026)
C.7.3(0024)
0.2.0(K76-0-9)
C.5.0(0004E0F0)
C.90.1(0000000030863358)
!

*/
// state of process
enum { ST_OK = 0, ST_RUN, ST_ERR, ST_DONE };

// Class to handle ascii <--> numeric values
class S_V
{
public:
	const char* signature;	// signature to get data, as "1.8.2"
	char* sval;				// ASCII value associated, as "230"
	float val;				// numeric value

	char* get_sval();		// use direct _buf, and cut at first no-numeric chr
};	// sruct S_V

class S_VD : public S_V	// same, for double value
{
public:
	//const char* signature;	// signature to get data, as "1.8.2"
	//char* sval;				// ASCII value associated, as "230"
	double val;				// numeric double value

	//char* get_sval();		// use direct _buf, and cut at first no-numeric chr
};	// sruct S_VD

// Class to handle all useful datas delivered by electronic counter
class Data
{
public:
	double e_consumed_0, e_producted_0;
	float p_consumed, p_producted;		// calculated for 15 minutes

	S_VD e_consumed, e_producted;		// energy
	S_V u_ph1, u_ph2, u_ph3; 
	float u;	// 3 phases voltage
	S_V i_ph1, i_ph2, i_ph3;
	float i;	// current
	S_V p_ph1, p_ph2, p_ph3; 
	float p;	// power
	S_V f_ph1, f_ph2, f_ph3; 
	char f[3];	// only sign is useful

	int state;

#define NB_S_VD (2)	// 2 energy
	S_VD * e_values[NB_S_VD] =
	{
		&e_consumed, &e_producted,
	};

#define NB_S_V (12)	// 3 u, 3 i, 3 p, 3 pf = 14 elements

	S_V * values[NB_S_V] =
	{
		&u_ph1, &u_ph2, &u_ph3,
		&i_ph1, &i_ph2, &i_ph3,
		&p_ph1, &p_ph2, &p_ph3,
		&f_ph1, &f_ph2, &f_ph3,
	};

	// Sets signatures of wanted element 
	void set_signatures()
	{
		e_consumed.signature = "1.8.0";	e_producted.signature = "2.8.0";
		u_ph1.signature = "32.7.0";;  u_ph2.signature = "52.7.0";;  u_ph3.signature = "72.7.0";;
		i_ph1.signature = "31.7.0";;  i_ph2.signature = "51.7.0";;  i_ph3.signature = "71.7.0";;
		p_ph1.signature = "36.7.0";;  p_ph2.signature = "56.7.0";;  p_ph3.signature = "76.7.0";;
		f_ph1.signature = "33.7.0";;  f_ph2.signature = "53.7.0";;  f_ph3.signature = "73.7.0";
	}

	// the sens of energy is given by the power factor f. 
	// If negative, the energy is pushed. Current and power are involved.
	void set_sense()
	{
		i_ph1.val *= f[0]; i_ph2.val *= f[1]; i_ph3.val *= f[2];
		p_ph1.val *= f[0]; p_ph2.val *= f[1]; p_ph3.val *= f[2];
	};

	// compute global values
	void avg_u() { u = (u_ph1.val + u_ph2.val + u_ph3.val) / 3.0; }
	void sum_i() { i = (i_ph1.val + i_ph2.val + i_ph3.val) ; }
	void sum_p() { p = (p_ph1.val + p_ph2.val + p_ph3.val) ; }

	void global_val()
	{
		avg_u(); sum_i(); sum_p();
	}

	/* 	energy_diff()
		------------
		Called each 15 minutes, compute energy difference during 1/4 hour
		This difference is multiplied by 4, to have the power average
	*/
	void energy_diff()
	{
		p_consumed = 4.0 * ((double)e_consumed.val - e_consumed_0);
		e_consumed_0 = e_consumed.val;

		p_producted = 4.0 * ((double)e_producted.val - e_producted_0);
		e_producted_0 = e_producted.val;
	}


#define S_V_ATOF(DATA_NAME) (DATA_NAME.val = atof(DATA_NAME.sval))

	// conversion from ASCII value to numeric floating point.
	int conv()
	{
		e_consumed.val = atof(e_consumed.sval);
		e_producted.val = atof(e_producted.sval);

		S_V_ATOF(u_ph1);
		S_V_ATOF(u_ph2);
		S_V_ATOF(u_ph3);

		S_V_ATOF(i_ph1);
		S_V_ATOF(i_ph2);
		S_V_ATOF(i_ph3);

		S_V_ATOF(p_ph1);
		S_V_ATOF(p_ph2);
		S_V_ATOF(p_ph3);

		// get sens of energy, by power factor f
		f[0] = *(f_ph1.sval) == '-' ? -1 : 1;
		f[1] = *(f_ph2.sval) == '-' ? -1 : 1;
		f[2] = *(f_ph3.sval) == '-' ? -1 : 1;

		set_sense();			// give to I an P the sens
		global_val();		// compute average of 3 phases

		return ST_OK;
	} // conv()


	// scan buffer and sets the ASCII pointers
	int populate(char* p_buf)
	{
		char* pscan;	// pointer to scan the buffer
		int i = 0; 		// index of element
		int miss = 0;	// counter of segment without find success

		set_signatures();		// each element does have its signature
		state = ST_RUN;

		// scan the entire buffer, to find signature
		// scan first, for element 'energy' stored in double
		pscan = p_buf;
		for (i = 0; (i < NB_S_VD) && (pscan < p_buf+E230_BUF_SZ) && (miss < 4);)
		{
			S_VD * v = e_values[i];	// point struct value to found
			v->sval = NULL;	// clear value of string value pointer
			v->val = 0.0;	// clear numeric value

			if (strncmp(pscan, v->signature, strlen(v->signature)) == 0)	// q: element found by signature?
			{
				pscan = pscan + strlen(v->signature) + 1;	// go ahead the '('
				v->sval = pscan;					// set sval to the next field
				pscan = pscan + strlen(pscan) + 1;	// go ahead the \0

				i++;		// next element to find
				miss = 0;	// reset miss counter
			}
			else
			{
				pscan = pscan + strlen(pscan) + 1;	// skip the field
				if (*pscan == '\0') miss++;

			}

		} // for buffer scan 


		// scan for next elements stored in float, follow in the buffer
		for (i = 0; (i < NB_S_V) && (pscan < p_buf+E230_BUF_SZ) && (miss < 4);)
		{
			S_V * v = values[i];	// point struct value to found
			v->sval = NULL;			// clear value of string value pointer
			v->val = 0.0;			// clear numeric value

			if (strncmp(pscan, v->signature, strlen(v->signature)) == 0)	// Q: element found by signature?
			{																// A: yes, 
				pscan = pscan + strlen(v->signature) + 1;	// go after the '('
				v->sval = pscan;							// set sval to the next field
				pscan = pscan + strlen(pscan) + 1;			// go ahead the \0, for next element

				i++;		// next element to find
				miss = 0;	// reset miss counter
			}
			else
			{
				pscan = pscan + strlen(pscan) + 1;	// skip this field
				if (*pscan == '\0') miss++;
			}

		} // for rest of buffer to scan 

		// check if all wanted signatures are found: state set
		if (i == NB_S_V) state = ST_DONE; else state = ST_ERR;
		// Serial.print("\nElements found:"); Serial.println(i);

		return state;
	}// populate()

/*	void print()
	------------
	Delayed function, to output onto serial the last values with the command 'p'.
*/
	void print()
	{		
#if 1		
		if (state == ST_ERR)
		{
			stream_out->print(F("Status:error\n"));
		}
		else if (state == ST_RUN)
		{
			stream_out->print(F("Run\n"));
		}
		else
		{
			char buf[21];	// local buffer for float to ASCII

			stream_out->print(F("E cons: ")); dtostrf(e_consumed.val, 10, 3, buf);
			stream_out->println(buf);			
			stream_out->print(F("E prod: "));	dtostrf(e_producted.val, 10, 3, buf);
			stream_out->println(buf);	


			stream_out->print(F("U ph1: ")); dtostrf(u_ph1.val, 3, 0, buf); stream_out->println(buf);
			stream_out->print(F("U ph2: ")); dtostrf(u_ph2.val, 3, 0, buf); stream_out->println(buf);
			stream_out->print(F("U ph3: ")); dtostrf(u_ph3.val, 3, 0, buf); stream_out->println(buf);

			stream_out->print(F("I ph1: ")); dtostrf(i_ph1.val, 5, 2, buf); stream_out->println(buf);
			stream_out->print(F("I ph2: ")); dtostrf(i_ph2.val, 5, 2, buf); stream_out->println(buf);
			stream_out->print(F("I ph3: ")); dtostrf(i_ph3.val, 5, 2, buf); stream_out->println(buf);

			stream_out->print(F("P ph1: ")); dtostrf(p_ph1.val, 5, 2, buf); stream_out->println(buf);
			stream_out->print(F("P ph2: ")); dtostrf(p_ph2.val, 5, 2, buf); stream_out->println(buf);
			stream_out->print(F("P ph3: ")); dtostrf(p_ph3.val, 5, 2, buf); stream_out->println(buf);

			// alway 0.0, only sign is used
			//stream_out->print(F("sens 1: ")); dtostrf(f_ph1.val, 5, 2, buf); stream_out->println(buf);
			//stream_out->print(F("sens 2: ")); dtostrf(f_ph2.val, 5, 2, buf); stream_out->println(buf);
			//stream_out->print(F("sens 3: ")); dtostrf(f_ph3.val, 5, 2, buf); stream_out->println(buf);

			dtostrf(u, 3, 0, buf);
			stream_out->print(F("U : ")); stream_out->println(buf);

			dtostrf(i, 5, 2, buf);
			stream_out->print(F("I : ")); stream_out->println(buf);

			dtostrf(p, 5, 2, buf);
			stream_out->print(F("P : ")); stream_out->println(buf);
		}
#else
		stream_out->print(F("N.A.\n"));
#endif
	} // print()

};	// class Data

CLASS Data data;	// static instance

#endif //E_CALC_HPP
