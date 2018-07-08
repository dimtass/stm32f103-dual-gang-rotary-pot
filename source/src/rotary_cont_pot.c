/*
 * rotary_cont_pot.c
 *
 *  Created on: Jul 5, 2018
 *      Author: Dimitris Tassopoulos
 */

#include "rotary_cont_pot.h"

#define ADC_HALF(INDEX,ADC_INDEX) ((m_pots[INDEX].settings[ADC_INDEX].max_adc_val - m_pots[INDEX].settings[ADC_INDEX].min_adc_val) >> 1)
#define IS_INCR(VAL1,VAL2) ((VAL1-VAL2) > 0 ? 1 : 0)
#define IS_DEADZONE(INDEX,ADC_INDEX,VAL) ( \
			(VAL > (m_pots[INDEX].data[ADC_INDEX].curr_adc_value - m_pots[index].settings[ADC_INDEX].dead_zone)) \
			&& (VAL < (m_pots[INDEX].data[ADC_INDEX].curr_adc_value + m_pots[index].settings[ADC_INDEX].dead_zone)) \
					)

enum en_rcp_error {
	RCP_ERROR_ALREADY_INIT = 1,
	RCP_ERROR_NOT_INIT,
	RCP_ERROR_MEMORY,
	RCP_MAX_POTS,

};

enum en_rcp_quarters {
	RCP_Q1,
	RCP_Q2,
	RCP_Q3,
	RCP_Q4
};

enum en_adc_num {
	RCP_ADC1 = 0,
	RCP_ADC2
};

/**
 * Rotary continuous pot data
 */
struct rcp_data {
	uint16_t	curr_adc_value;
	uint16_t	prev_adc_value;
};


/**
 * Rotary continuous pot data
 * min				: That's the the min value that the pot can take.
 * max				: That's the max value of the pot
 * min_adc_val		: That's the min value that the ADC can measure
 * max_adc_val		: That's the max value that the ADC can measure
 * adc_step			: This value defines the ADC window for which the
 * 						relative value doesn't change. So, if from the
 * 						(curr_value-adc_step) to (curr_adc_value+adc_step)
 * 						the relative pot value is the same
 */
struct rcp_pot {
	tp_rcp_val		value;
	tp_rcp_val		min;
	tp_rcp_val		max;
	tp_rcp_val		step;
	uint8_t			prev_quarter;
	struct rcp_settings settings[2];
	struct rcp_data  	data[2];
};

/* pointer to array of settings and data structures */
static struct rcp_pot *m_pots = NULL;
static uint8_t m_max_pots = 0;	// max number of supported pots
static uint8_t m_next_available_pot = 0;	// when this reaches m_max_pots-1 then no other pots are available

/**
 *
 */
int rcp_init(uint8_t num_of_pots)
{
	if (m_pots) return -RCP_ERROR_ALREADY_INIT;

	m_max_pots = num_of_pots;
	m_pots = malloc(m_max_pots * sizeof(struct rcp_pot));
	if (!m_pots) {
		return -RCP_ERROR_MEMORY;
	}
	TRACE(("Created %d pots\n", num_of_pots));

	return 0;
}

/**
 *
 */
int rcp_add(uint16_t adc1_val, uint16_t adc2_val, tp_rcp_val start_value,
		tp_rcp_val min, tp_rcp_val max, tp_rcp_val step,
		struct rcp_settings *adc1_settings, struct rcp_settings *adc2_settings)
{
	int index = 0;

	if (!m_pots)
		return -RCP_ERROR_ALREADY_INIT;

	/* Enough slots? */
	if (m_next_available_pot >= m_max_pots)
		return -RCP_MAX_POTS;

	uint8_t i = m_next_available_pot;

	m_pots[i].value = start_value;
	m_pots[i].min = min;
	m_pots[i].max = max;
	m_pots[i].step = step;
	memcpy(&m_pots[i].settings[RCP_ADC1], adc1_settings, sizeof(struct rcp_settings));
	memcpy(&m_pots[i].settings[RCP_ADC2], adc2_settings, sizeof(struct rcp_settings));

	m_pots[i].data[RCP_ADC1].curr_adc_value = adc1_val;
	m_pots[i].data[RCP_ADC1].prev_adc_value = adc1_val;

	m_pots[i].data[RCP_ADC2].curr_adc_value = adc2_val;
	m_pots[i].data[RCP_ADC2].prev_adc_value = adc2_val;

	TRACE(("Added pot with min:%.2f max:%.2f\n", m_pots[m_next_available_pot].min, m_pots[m_next_available_pot].max));
	m_next_available_pot++;

	return index;
}


static inline uint8_t rcp_get_quarter(uint8_t index, uint16_t adc1_val, uint16_t adc2_val)
{
	uint8_t quarter = RCP_Q1;

	if ( (adc1_val <= ADC_HALF(index,RCP_ADC1)) &&
			(adc2_val >= ADC_HALF(index,RCP_ADC2)) ) {
		quarter = RCP_Q1;
	}
	else if ( (adc1_val >= ADC_HALF(index,RCP_ADC1)) &&
			(adc2_val >= ADC_HALF(index,RCP_ADC2)) ) {
		quarter = RCP_Q2;
	}
	else if ( (adc1_val >= ADC_HALF(index,RCP_ADC1)) &&
			(adc2_val <= ADC_HALF(index,RCP_ADC2)) ) {
		quarter = RCP_Q3;
	}
	else if ( (adc1_val <= ADC_HALF(index,RCP_ADC1)) &&
			(adc2_val <= ADC_HALF(index,RCP_ADC2)) ) {
		quarter = RCP_Q4;
	}

	return quarter;
}

static inline void rcp_increment_value(uint8_t index)
{
	if (m_pots[index].value < m_pots[index].max) {
		tp_rcp_val tmp = m_pots[index].value;
		tmp += m_pots[index].step;
		if (tmp > m_pots[index].max)
			tmp = m_pots[index].max;
		m_pots[index].value = tmp;
	}
	TRACE(("[+]: %.2f\n", (float) m_pots[index].value));
}

static inline void rcp_decrement_value(uint8_t index)
{
	if (m_pots[index].value > m_pots[index].min) {
		tp_rcp_val tmp = m_pots[index].value;
		tmp -= m_pots[index].step;
		if (tmp < m_pots[index].min)
			tmp = m_pots[index].min;
		m_pots[index].value = tmp;
	}
	TRACE(("[-]: %.2f\n", (float) m_pots[index].value));
}


int rcp_set_update_adc_values(uint8_t index, uint16_t adc1_val, uint16_t adc2_val)
{
	if (index >= m_max_pots) return -1;

	uint8_t quarter = rcp_get_quarter(index, adc1_val, adc2_val);

//	if (quarter != m_pots[index].prev_quarter) {
//
//	}
//	else if (IS_DEADZONE(index,RCP_ADC1,adc1_val) || IS_DEADZONE(index,RCP_ADC2,adc2_val))
//		return -2;

	uint16_t prev1 = m_pots[index].data[RCP_ADC1].curr_adc_value;
	uint16_t curr1 = adc1_val;
	uint16_t prev2 = m_pots[index].data[RCP_ADC2].curr_adc_value;
	uint16_t curr2 = adc2_val;

	if (quarter == RCP_Q1) {
		if (IS_DEADZONE(index,RCP_ADC2,adc2_val))
			return -2;
		if ( IS_INCR(curr2,prev2)
				|| (m_pots[index].prev_quarter == RCP_Q4))
			rcp_increment_value(index);
		else
			rcp_decrement_value(index);
	}
	else if (quarter == RCP_Q2) {
		if (IS_DEADZONE(index,RCP_ADC1,adc1_val))
			return -2;
		if ( IS_INCR(curr1,prev1)
				|| (m_pots[index].prev_quarter == RCP_Q1))
			rcp_increment_value(index);
		else
			rcp_decrement_value(index);
	}
	else if (quarter == RCP_Q3) {
		if (IS_DEADZONE(index,RCP_ADC1,adc1_val))
			return -2;
		if ( !IS_INCR(curr1,prev1)
			|| (m_pots[index].prev_quarter == RCP_Q2))
			rcp_increment_value(index);
		else
			rcp_decrement_value(index);
	}
	else if (quarter == RCP_Q4) {
		if (IS_DEADZONE(index,RCP_ADC2,adc2_val))
			return -2;
		if ( IS_INCR(curr2,prev2)
				|| (m_pots[index].prev_quarter == RCP_Q3))
			rcp_increment_value(index);
		else
			rcp_decrement_value(index);
	}
//	TRACE(("%d: [1]:%d,[2]:%d\n", quarter, curr1-prev1, curr2-prev2));

	/* update prev/curr values */
	m_pots[index].prev_quarter = quarter;
	m_pots[index].data[RCP_ADC1].prev_adc_value = prev1;
	m_pots[index].data[RCP_ADC1].curr_adc_value = curr1;
	m_pots[index].data[RCP_ADC2].prev_adc_value = prev2;
	m_pots[index].data[RCP_ADC2].curr_adc_value = curr2;

	return 0;
}

tp_rcp_val rcp_get_value(uint8_t index)
{
	return m_pots[index].value;
}

void rcp_set_value(uint8_t index, tp_rcp_val value)
{
	if ((value >= m_pots[index].min) && (value <= m_pots[index].max)) {
		m_pots[index].value = value;
	}
}
