//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "You Zhou";
const char *studentID = "yoz023";
const char *email = "yoz023@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 16; // Number of bits used for Global History

// variables for tournament
uint32_t lht_entries = 1 << 12; // local history table in tournament
uint32_t lpt_entries = 1 << 12; // local prediction table in tournament
uint32_t gpt_entries = 1 << 14; // global prediction table in tournament
uint32_t cpt_entries = 1 << 14; // choice prediction table in tournament

// variables for TAGE

int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
// tournament
uint16_t *lht_tournament;
uint8_t *lpt_tournament;
uint8_t *gpt_tournament;
uint8_t *cpt_tournament;
// TAGE
uint32_t **tables;
uint32_t num_table = 5;
uint32_t tage_len = 10;
uint32_t base_len = 8;
uint32_t bimo_len = 12;
uint32_t tag_len = 8;
bool update_u = false;
uint8_t pred = NOTTAKEN;


uint64_t ghistory;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}
// shew
uint8_t *bank1;
uint32_t one_entries = 1 << 14;
uint8_t *bank2;
uint32_t two_entries = 1 << 14;
uint8_t *bank3;
uint32_t three_entries = 1 << 14;
uint32_t shift1 = 9;
uint32_t shift2 = 24;
void init_skew()
{
	int i;
	bank1 = (uint8_t *)malloc(one_entries * sizeof(uint8_t));
	for (i = 0; i < one_entries; i++)
	{
		bank1[i] = WN;
	}
	bank2 = (uint8_t *)malloc(two_entries * sizeof(uint8_t));
	for (i = 0; i < two_entries; i++)
	{
		bank2[i] = WN;
	}
	bank3 = (uint8_t *)malloc(three_entries * sizeof(uint8_t));
	for (i = 0; i < three_entries; i++)
	{
		bank3[i] = WN;
	}
	ghistory = 0;
}

uint8_t skew_predict(uint32_t pc)
{
	uint32_t one_index, two_index, three_index;
  uint64_t base_index;
  uint8_t one_pred, two_pred, three_pred;
	base_index = pc ^ ghistory;
  one_index = base_index & (one_entries - 1);
  one_pred = bank1[one_index] >> 1;
  two_index = (ghistory << shift1) | (ghistory >> (64 - shift1));
  two_index = ((two_index ^ pc) * 0x6484CDA47D757D59) & (two_entries - 1);
  two_pred = bank2[two_index] >> 1;
  three_index = (ghistory << shift2) | (ghistory >> (64 - shift2));
  three_index = ((three_index ^ pc) * 0x3FF9AEC04B678BAD) & (three_entries - 1);
  three_pred = bank3[three_index] >> 1;
  return (one_pred + two_pred + three_pred) >> 1;
}

void train_skew(uint32_t pc, uint8_t outcome)
{
  uint32_t one_index, two_index, three_index;
  uint64_t base_index;
  uint8_t one_pred, two_pred, three_pred;
	base_index = pc ^ ghistory;
  one_index = base_index & (one_entries - 1);
  one_pred = bank1[one_index] >> 1;
  two_index = (ghistory << shift1) | (ghistory >> (64 - shift1));
  two_index = ((two_index ^ pc) * 0x6484CDA47D757D59) & (two_entries - 1);
  two_pred = bank2[two_index] >> 1;
  three_index = (ghistory << shift2) | (ghistory >> (64 - shift2));
  three_index = ((three_index ^ pc) * 0x3FF9AEC04B678BAD) & (three_entries - 1);
  three_pred = bank3[three_index] >> 1;
  uint8_t vote = (one_pred + two_pred + three_pred) >> 1;
  if (vote != outcome)
  {
    if (outcome == TAKEN) {
      bank1[one_index] = (bank1[one_index] + 1 < ST) ? bank1[one_index] + 1 : ST;
      bank2[two_index] = (bank2[two_index] + 1 < ST) ? bank2[two_index] + 1 : ST;
      bank3[three_index] = (bank3[three_index] + 1 < ST) ? bank3[three_index] + 1 : ST;
    } else {
      bank1[one_index] = (bank1[one_index] - 1 > SN) ? bank1[one_index] - 1 : SN;
      bank2[two_index] = (bank2[two_index] - 1 > SN) ? bank2[two_index] - 1 : SN;
      bank3[three_index] = (bank3[three_index] - 1 > SN) ? bank3[three_index] - 1 : SN;
    }
  }
  else
  {
    if (outcome == TAKEN) {
      if (one_pred == outcome) {
        bank1[one_index] = (bank1[one_index] + 1 < ST) ? bank1[one_index] + 1 : ST;
      }
      if (two_pred == outcome) {
        bank2[two_index] = (bank2[two_index] + 1 < ST) ? bank2[two_index] + 1 : ST;
      }
      if (three_pred == outcome) {
        bank3[three_index] = (bank3[three_index] + 1 < ST) ? bank3[three_index] + 1 : ST;
      }
    } else {
      if (one_pred == outcome) {
        bank1[one_index] = (bank1[one_index] - 1 > SN) ? bank1[one_index] - 1 : SN;
      }
      if (one_pred == outcome) {
        bank2[two_index] = (bank2[two_index] - 1 > SN) ? bank2[two_index] - 1 : SN;
      }
      if (one_pred == outcome) {
        bank3[three_index] = (bank3[three_index] - 1 > SN) ? bank3[three_index] - 1 : SN;
      }
    }
  }
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_skew()
{
	free(bank1);
	free(bank2);
	free(bank3);
}

// skew tournament
/*
void init_skew_tournament()
{
  lht_entries = 1 << 11;//11; // local history table in tournament
  lpt_entries = 1 << 10;//10; // local prediction table in tournament
  cpt_entries = 1 << 12;//12; // choice prediction table in tournament
  lht_tournament = (uint16_t *)malloc(lht_entries * sizeof(uint16_t));  // local history table
  lpt_tournament = (uint8_t *)malloc(lpt_entries * sizeof(uint8_t));    // local prediction table
  gpt_tournament = (uint8_t *)malloc(gpt_entries * sizeof(uint8_t));    // global prediction table

  // 'Taken' for choice table means take the local prediction
  cpt_tournament = (uint8_t *)malloc(cpt_entries * sizeof(uint8_t));    // choice prediction table

  int i = 0;
  for (i = 0; i < lht_entries; i++)
  {
    lht_tournament[i] = 0;
  }
  for (i = 0; i < lpt_entries; i++)
  {
    lpt_tournament[i] = WN;
  }
  for (i = 0; i < gpt_entries; i++)
  {
    gpt_tournament[i] = WN;
  }
  for (i = 0; i < cpt_entries; i++)
  {
    cpt_tournament[i] = WN;
  }
  ghistory = 0;
  init_skew();
}
*/
uint8_t *choicer;
void init_skew_tournament()
{
  ghistoryBits = 13;
  init_gshare();
  init_skew();
  choicer = (uint8_t *)malloc((1 << 13) * sizeof(uint8_t));
  int i;
  for (i = 1; i < (1 << 13); i++)
  {
    choicer[i] = WN;
  }
}
uint8_t skew_tournament_predict(uint32_t pc) {
  uint32_t choicer_index = 1 << 13;
  choicer_index = (ghistory ^ pc) & (choicer_index - 1);
  if (choicer[choicer_index] >= WT)
  {
    return gshare_predict(pc);
  }
  else
  {
    return skew_predict(pc);
  }

}
/*
uint8_t skew_tournament_predict(uint32_t pc)
{
  uint32_t gp_index = (ghistory ^ pc) & (gpt_entries - 1);
  uint32_t cp_index = (ghistory ^ pc) & (cpt_entries - 1);
  uint32_t lh_index = ghistory & (lht_entries - 1);
  uint32_t lp_index = lht_tournament[lh_index] & (lpt_entries - 1);
  if (cpt_tournament[cp_index] >= WT)
  {
    // take the local table
    return lpt_tournament[lp_index] >> 1;
  }
  else
  {
    // take the global table
    return skew_predict(pc);
  }
}
*/

void train_skew_tournament(uint32_t pc, uint8_t outcome)
{
  uint64_t ghcopy = ghistory;
  uint32_t choicer_index = 1 << 13;
  choicer_index = (ghistory ^ pc) & (choicer_index - 1);
  uint8_t skew_result = skew_predict(pc);
  uint8_t gshare_result = gshare_predict(pc);
  if (skew_result == outcome && gshare_result != outcome)
  {
    if (choicer[choicer_index] > 0) {
      choicer[choicer_index]--;
    }
  }
  if (skew_result != outcome && gshare_result == outcome)
  {
    if (choicer[choicer_index] < 3) {
      choicer[choicer_index]++;
    }
  }
  train_skew(pc, outcome);
  ghistory = ghcopy;
  train_gshare(pc, outcome);
  ghistory = ghcopy;
  ghistory = ((ghcopy << 1) | outcome);
}

/*
void train_skew_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t gp_index = (ghistory ^ pc) & (gpt_entries - 1);
  uint32_t cp_index = (ghistory ^ pc) & (cpt_entries - 1);
  uint32_t lh_index = ghistory & (lht_entries - 1);
  uint32_t lp_index = lht_tournament[lh_index] & (lpt_entries - 1);

  uint8_t gprediction = gpt_tournament[gp_index] >> 1;
  uint8_t lprediction = lpt_tournament[lp_index] >> 1;
  // update state of entry in the prediction tables
  if (outcome == TAKEN)
  {
    lpt_tournament[lp_index] = (lpt_tournament[lp_index] + 1 < ST) ? lpt_tournament[lp_index] + 1 : ST;
    gpt_tournament[gp_index] = (gpt_tournament[gp_index] + 1 < ST) ? gpt_tournament[gp_index] + 1 : ST;
  }
  else
  {
    lpt_tournament[lp_index] = (lpt_tournament[lp_index] - 1 > SN) ? lpt_tournament[lp_index] - 1 : SN;
    gpt_tournament[gp_index] = (gpt_tournament[gp_index] - 1 > SN) ? gpt_tournament[gp_index] - 1 : SN;
  }
  train_skew(pc, outcome);

  // update state of entry in the choice table

  if ((gprediction == outcome) && (lprediction != outcome))
  {
    cpt_tournament[cp_index] = (cpt_tournament[cp_index] - 1 > SN) ? cpt_tournament[cp_index] - 1 : SN;
  } else if ((gprediction != outcome) && (lprediction == outcome)) 
  {
    cpt_tournament[cp_index] = (cpt_tournament[cp_index] + 1 < ST) ? cpt_tournament[cp_index] + 1 : ST;
  }

  // update history register
  //ghistory = ((ghistory << 1) | outcome);
  lht_tournament[lh_index] = ((lp_index << 1) | outcome);
}
*/
void cleanup_skew_tournament()
{
  cleanup_skew();
  cleanup_gshare();
  //free(lht_tournament);
  //free(lpt_tournament);
  //free(gpt_tournament);
  //free(cpt_tournament);
}


// bimode
uint8_t *choice_table;
uint32_t choice_entries = 1 << 16;
uint8_t *taken_table;
uint32_t taken_entries = 1 << 16;
uint8_t *nottaken_table;
uint32_t nottaken_entries = 1 << 16;

void init_bimode()
{
	choice_table = (uint8_t *)malloc(choice_entries * sizeof(uint8_t));
	taken_table = (uint8_t *)malloc(taken_entries * sizeof(uint8_t));
	nottaken_table = (uint8_t *)malloc(nottaken_entries * sizeof(uint8_t));
	int i;
	for (i = 0; i < choice_entries; i++)
	{
		choice_table[i] = WN;
	}
	for (i = 0; i < taken_entries; i++)
	{
		taken_table[i] = WT;
	}
	for (i = 0; i < nottaken_entries; i++)
	{
		nottaken_table[i] = WN;
	}
	ghistory = 0;
}

uint8_t choice_prediction;
uint8_t direction_prediction;
uint32_t bindex;
uint8_t bimode_predict(uint32_t pc)
{
	bindex = pc ^ ghistory;
	choice_prediction = choice_table[bindex & (choice_entries - 1)] >> 1;
	if (choice_prediction == 1)
	{
		direction_prediction = taken_table[bindex & (taken_entries - 1)] >> 1;
		return direction_prediction;
	}
	else
	{
		direction_prediction = nottaken_table[bindex & (nottaken_entries - 1)] >> 1;
		return direction_prediction;
	}
}

void train_bimode(uint32_t pc, uint8_t outcome)
{

	if ((choice_prediction == outcome) || (direction_prediction != outcome))
	{
		// update the choice table
		if (outcome == TAKEN)
		{
			if (choice_table[bindex & (choice_entries - 1)] < ST)
			{
				choice_table[bindex & (choice_entries - 1)]++;
			}
		}
		else
		{
			if (choice_table[bindex & (choice_entries - 1)] > SN)
			{
				choice_table[bindex & (choice_entries - 1)]--;
			}
		}
	}
	if (choice_prediction == 1)
	{
		// choose the taken table
		if (outcome == TAKEN)
		{
			if (taken_table[bindex & (taken_entries - 1)] < ST)
			{
				taken_table[bindex & (taken_entries - 1)]++;
			}
		}
		else
		{
			if (taken_table[bindex & (taken_entries - 1)] > SN)
			{
				taken_table[bindex & (taken_entries - 1)]--;
			}
		}
	}
	else
	{
		// choose the nottaken table
		if (outcome == TAKEN)
		{
			if (nottaken_table[bindex & (nottaken_entries - 1)] < ST)
			{
				nottaken_table[bindex & (nottaken_entries - 1)]++;

			}
		}
		else
		{
			if (nottaken_table[bindex & (nottaken_entries - 1)] > SN)
			{
				nottaken_table[bindex & (nottaken_entries - 1)]--;
			}
		}
	}
	ghistory = ((ghistory << 1) | outcome);
}

void cleanup_bimode()
{
	free(choice_table);
	free(taken_table);
	free(nottaken_table);
}


// TAGE functions
void init_TAGE()
{
  tables = (uint32_t **)malloc(num_table * sizeof(uint32_t **));
  uint32_t bimo_entries = 1 << bimo_len;
  uint32_t tage_entries = 1 << tage_len;
  int i;
  int j;
  // initialize table 0
  tables[0] = (uint32_t *)malloc(bimo_entries * sizeof(uint32_t));
  for (i = 0; i < bimo_entries; i++)
  {
    tables[0][i] = WN;
  }
  for (i = 1; i < num_table; i++)
  {
    tables[i] = (uint32_t *)malloc(tage_entries  * sizeof(uint32_t));
    for (j=0; j < tage_entries; j++)
    {
      tables[i][j] = 3 << (tag_len + 2);
    }
  }
}

uint32_t fold(uint32_t his_len, uint32_t res_len)
{
  uint64_t gh_copy = ghistory & ((1 << his_len) - 1);
  uint32_t res = 0;
  while (gh_copy != 0)
  {
    res = res ^ (gh_copy & ((1 << res_len) - 1));
    gh_copy = gh_copy >> res_len;
  }
  return res;
}

uint8_t TAGE_predict(uint32_t pc)
{
  uint32_t provider = 0;
  int i;
  uint32_t his_len = base_len;
  uint32_t index, tag1, tag2, provider_entry;
  uint8_t defprediction = tables[0][pc & ((1 << bimo_len) - 1)] >> 1;
  uint8_t altprediction = defprediction;
  uint8_t finprediction = defprediction;
  update_u = false;
  for (i = 1; i < num_table; i++)
  {
    index = (pc & ((1 << tage_len) - 1)) ^ fold(his_len, tage_len);
    tag1 = (tables[i][index] >> 2) & ((1 << tag_len) - 1);
    tag2 = (pc & ((1 << tag_len) - 1)) ^ fold(his_len, tag_len) ^ (fold(his_len, tag_len - 1) << 1);
    his_len = his_len * 2;
    if (tag1 == tag2)
    {
      altprediction = finprediction;
      finprediction = tables[i][index] >> (tag_len + 2 + 2);
      provider = i;
    }
  }
  if (provider == 0){
    pred = defprediction;
    return defprediction;
  } else {
    if (altprediction != finprediction) {
      update_u = true;
    }
    pred = defprediction;
    return finprediction;
  }
}

void train_TAGE(uint32_t pc, uint8_t outcome)
{
  if (update_u)
  {
    if (pred != outcome) {

    }
  }

}





// tournament functions

void init_tournament()
{
  lht_tournament = (uint16_t *)malloc(lht_entries * sizeof(uint16_t));  // local history table
  lpt_tournament = (uint8_t *)malloc(lpt_entries * sizeof(uint8_t));    // local prediction table
  gpt_tournament = (uint8_t *)malloc(gpt_entries * sizeof(uint8_t));    // global prediction table

  // 'Taken' for choice table means take the local prediction
  cpt_tournament = (uint8_t *)malloc(cpt_entries * sizeof(uint8_t));    // choice prediction table

  int i = 0;
  for (i = 0; i < lht_entries; i++)
  {
    lht_tournament[i] = 0;
  }
  for (i = 0; i < lpt_entries; i++)
  {
    lpt_tournament[i] = WN;
  }
  for (i = 0; i < gpt_entries; i++)
  {
    gpt_tournament[i] = WN;
  }
  for (i = 0; i < cpt_entries; i++)
  {
    cpt_tournament[i] = WN;
  }
  ghistory = 0;
}

uint8_t tournament_predict(uint32_t pc)
{
  uint32_t gp_index = (ghistory ^ pc) & (gpt_entries - 1);
  uint32_t cp_index = (ghistory ^ pc) & (cpt_entries - 1);
  uint32_t lh_index = pc & (lht_entries - 1);
  uint32_t lp_index = lht_tournament[lh_index] & (lpt_entries - 1);
  if (cpt_tournament[cp_index] >= WT)
  {
    // take the local table
    return lpt_tournament[lp_index] >> 1;
  }
  else
  {
    // take the global table
    return gpt_tournament[gp_index] >> 1;
  }
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t gp_index = (ghistory ^ pc) & (gpt_entries - 1);
  uint32_t cp_index = (ghistory ^ pc) & (cpt_entries - 1);
  uint32_t lh_index = pc & (lht_entries - 1);
  uint32_t lp_index = lht_tournament[lh_index] & (lpt_entries - 1);

  uint8_t gprediction = gpt_tournament[gp_index] >> 1;
  uint8_t lprediction = lpt_tournament[lp_index] >> 1;
  // update state of entry in the prediction tables
  if (outcome == TAKEN)
  {
    lpt_tournament[lp_index] = (lpt_tournament[lp_index] + 1 < ST) ? lpt_tournament[lp_index] + 1 : ST;
    gpt_tournament[gp_index] = (gpt_tournament[gp_index] + 1 < ST) ? gpt_tournament[gp_index] + 1 : ST;
  }
  else
  {
    lpt_tournament[lp_index] = (lpt_tournament[lp_index] - 1 > SN) ? lpt_tournament[lp_index] - 1 : SN;
    gpt_tournament[gp_index] = (gpt_tournament[gp_index] - 1 > SN) ? gpt_tournament[gp_index] - 1 : SN;
  }

  // update state of entry in the choice table

  if ((gprediction == outcome) && (lprediction != outcome))
  {
    cpt_tournament[cp_index] = (cpt_tournament[cp_index] - 1 > SN) ? cpt_tournament[cp_index] - 1 : SN;
  } else if ((gprediction != outcome) && (lprediction == outcome)) 
  {
    cpt_tournament[cp_index] = (cpt_tournament[cp_index] + 1 < ST) ? cpt_tournament[cp_index] + 1 : ST;
  }

  // update history register
  ghistory = ((ghistory << 1) | outcome);
  lht_tournament[lh_index] = ((lp_index << 1) | outcome);
}

void cleanup_tournament()
{
  free(lht_tournament);
  free(lpt_tournament);
  free(gpt_tournament);
  free(cpt_tournament);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
	  //init_skew();
    init_skew_tournament();
    //init_bimode();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    //return skew_predict(pc);
    return skew_tournament_predict(pc);
    //return bimode_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      //return train_skew(pc, outcome);
      return train_skew_tournament(pc, outcome);
      //return train_bimode(pc, outcome);
    default:
      break;
    }
  }
}
