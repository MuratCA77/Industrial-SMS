

/* Detector analyses given window of signal power vs frequency in terms of
 * peak and average power levels calculated in three parts ("left", "central" and "right")
 * with adjustable window size for averaging. Detection is based on correspondance of 
 * relation of calculated levels to target value - so for different types of signals, 
 * different detectors can be defined, each with its own parameters.
 * */
typedef struct sSignalDetector
{
	char name[32]; //filled by user, optional
	float standard_min_frequency_MHz; //min frequency where this signal type should apper, lower frequencies will have decreased probability
	float standard_max_frequency_MHz; //max frequency where this signal type should apper, higher frequencies will have decreased probability

	float center_width_kHz; //width of central band in kHz
	float side_width_kHz; //width of "side" background level in kHz
	
	float detection_threshold; //threshold in dB, minimum signal level over noise to trigger detection procedure
	float bw_threshold; //bw calculation threshold
	
	float get_min_freq()
	{
		return center_width_kHz/2 + side_width_kHz;
	};
	float get_max_freq()
	{
		return center_width_kHz/2 + side_width_kHz;
	};
	int get_window_width_points(float frequency_step_hz)
	{
		return (2*side_width_kHz + center_width_kHz)*1000.0 / frequency_step_hz;
	};
/*process_data returns value from 0 to 1 indicating how well given signal fits detector's profile AT THE CENTER of provided frequency range.
 * this function doesn't scan given interval, scanning must be implemented outside of detector
 * If detector can't be applied (provided frequency range is smaller than detector parameters)
 * then it will return -1
 * 
 * Central band power is sent into res_power parameter
 * Also this function tries to detect channel BW, resulting width is stored into res_bw parameter
 * */
	float apply_detector(float *power_array, float frequency_start_hz, float frequency_step_hz, float current_frequency_hz, float *res_power, float *res_bw, float *res_centroid)
	{
		int center_pos = (current_frequency_hz - frequency_start_hz) / frequency_step_hz;
		int center_width = center_width_kHz * 1000.0 / frequency_step_hz;
		int side_width = side_width_kHz * 1000.0 / frequency_step_hz;
		int left_begin = center_pos - center_width/2 - side_width;
		int right_begin = center_pos + center_width/2 + side_width;
		
		float left_level = 0;
		float right_level = 0;
		for(int x = 0; x < side_width; x++)
		{
			left_level += power_array[left_begin + x];
			right_level += power_array[right_begin + x];
		}
		left_level /= (float)side_width;
		right_level /= (float)side_width;

		float center_level = 0;
		for(int x = 0; x < center_width; x++)
		{
			center_level += power_array[center_pos - center_width/2 + x];
		}
		center_level /= (float)center_width;
		
		if(center_level < left_level || center_level < right_level)
		{
			*res_power = 0;
			*res_bw = 0.0001;
			return 0;
		}

		float left_signal = 0;
		float right_signal = 0;
		float left_z = 0.000001;
		float right_z = 0.000001;
		for(int x = 0; x < 1 + center_width/2; x++)
		{
			float lv = power_array[center_pos - x];
			float rv = power_array[center_pos + x];
			if(lv > center_level)
			{
				left_signal += lv;
				left_z++;
			}
			if(rv > center_level)
			{
				right_signal += rv;
				right_z++;
			}
		}
		
		left_signal /= left_z;
		right_signal /= right_z;
		
		if(left_signal < left_level + detection_threshold) return 0;
		if(right_signal < right_level + detection_threshold) return 0;
		
		int left_signal_start = center_pos;
		int right_signal_start = center_pos;
//		printf("detecting: center %g left %g (%g) right %g (%g)\n", current_frequency_hz, left_signal, left_level, right_signal, right_level);
		for(int x = 0; x < 1 + center_width/2; x++)
		{
			float lv = power_array[center_pos - x];
			float rv = power_array[center_pos + x];
//			printf("x %d lv %g rv %g\n", x, lv, rv);
			if(lv < 0.7*left_signal + 0.3*left_level && left_signal_start == center_pos)
				left_signal_start = center_pos - x;
			if(rv < 0.7*right_signal + 0.3*right_level && right_signal_start == center_pos)
				right_signal_start = center_pos + x;
		}
		if(left_signal_start == center_pos) left_signal_start = center_pos - center_width/2;
		if(right_signal_start == center_pos) right_signal_start = center_pos + center_width/2;

		double pow_integr = 0.00000001;
		double freq_power_integr = 0.0;
		double pa_power_int = 0;
		for(int px = left_signal_start; px < right_signal_start; px++)
		{
			pa_power_int += power_array[px];
			double cur_pw = pow(10.0, power_array[px] * 0.1);
			pow_integr += cur_pw;
			double cur_fr = frequency_start_hz + px * frequency_step_hz;
			freq_power_integr += cur_fr * power_array[px];
		}

		*res_power = 10.0 * log10(pow_integr);
		*res_bw = (right_signal_start - left_signal_start) * frequency_step_hz;
		*res_centroid = freq_power_integr / pa_power_int;
		
		float score = left_level / left_signal - 1.0 + right_level / right_signal - 1.0;
		return score;
	}
}sSignalDetector;


/* Detector analyses given window of signal power vs frequency in terms of
 * peak and average power levels calculated in three parts ("left", "central" and "right")
 * with adjustable window size for averaging. Detection is based on correspondance of 
 * relation of calculated levels to target value - so for different types of signals, 
 * different detectors can be defined, each with its own parameters.
 * */
typedef struct sSignalDetector_old
{
	char name[32]; //filled by user, optional
	float standard_min_frequency_MHz; //min frequency where this signal type should apper, lower frequencies will have decreased probability
	float standard_max_frequency_MHz; //max frequency where this signal type should apper, higher frequencies will have decreased probability

/* For such reference signal profile:
 *	                   __
 *                    /  \
 *              _____/    \    _
 *             /           \__/ \
 * ___________/  ^             ^ \__________
 *               |   <---->    |
 *               |Center width |
 *               |      <----->|
 *               |<---->   Right shift
 *            Left shift
 * 
 *            <----->       <---->
 *          Left width      Right width
 * */	
	
	int single_peak;
	float center_width_kHz; //width of central band in kHz
	float left_shift_kHz; //shift of "left" part of signal peak in kHz, relative to center
	float left_width_kHz; //width of "left" band in kHz
	float right_shift_kHz; //shift of "right" part of signal peak in kHz, relative to center
	float right_width_kHz; //width of "right" band in kHz
	
	float left_relative_power; //power relative to central band, higher and lower detected values will decrease probability of detection
	float right_relative_power;

	float get_min_freq()
	{
		return left_shift_kHz + 2*left_width_kHz;
	};
	float get_max_freq()
	{
		return right_shift_kHz + 2*right_width_kHz;
	};
	int get_window_width_points(float frequency_step_hz)
	{
		return (left_shift_kHz + 2.1*left_width_kHz + right_shift_kHz + 2.1*right_width_kHz)*1000.0 / frequency_step_hz;
	};
/*process_data returns value from 0 to 1 indicating how well given signal fits detector's profile AT THE CENTER of provided frequency range.
 * this function doesn't scan given interval, scanning must be implemented outside of detector
 * If detector can't be applied (provided frequency range is smaller than detector parameters)
 * then it will return -1
 * 
 * Central band power is sent into res_power parameter
 * Also this function tries to detect channel BW, resulting width is stored into res_bw parameter
 * */
	float process_data(float *power_array, float frequency_start_hz, float frequency_step_hz, int length, float *res_power, float *res_bw)
	{
		float range_average = 0;
		float range_low_average = 0;

		float m1_length = 1.0 / (float)length;
		for(int x = 0; x < length; x++) range_average += power_array[x];
		range_average *= m1_length;
		float low_z = 0.00000000001;
		for(int x = 0; x < length; x++)
			if(power_array[x] < range_average) {
				range_low_average += power_array[x];
				low_z++;
			}
		range_low_average /= low_z;
		
		int center_pos = length/2;
		float center_freq = frequency_start_hz + frequency_step_hz * center_pos;
		float left_freq = center_freq - left_shift_kHz * 1000.0;
		float right_freq = center_freq + right_shift_kHz * 1000.0;
		
		int left_pos = (left_freq - frequency_start_hz) / frequency_step_hz;
		int right_pos = (right_freq - frequency_start_hz) / frequency_step_hz;

		int center_width = center_width_kHz * 1000.0 / frequency_step_hz;
		int left_width = left_width_kHz * 1000.0 / frequency_step_hz;
		int right_width = right_width_kHz * 1000.0 / frequency_step_hz;
		
		if(left_pos - 2*left_width < 0 || right_pos + 2*right_width >= length || center_width*2.1 > length) 
		{
			printf("left_pos %d right_pos %d left_width %d right_width %d length %d center_width %d", left_pos, right_pos, left_width, right_width, length, center_width);
			return -1;
		}
		
		float out_average = 0;
		float out_average_left = 0;
		float out_average_right = 0;
		float out_low_average = 0;
		float out_low_average_left = 0;
		float out_low_average_right = 0;
		float out_z = 0.00000001;
		float out_z_left = 0.00000001;
		float out_z_right = 0.00000001;
		float out_low_z = 0.00000001;
		float out_low_z_left = 0.00000001;
		float out_low_z_right = 0.00000001;
		for(int x = 0; x < left_pos - left_width; x++)
		{
			out_average_left += power_array[x];
			out_z_left++;
			out_average += power_array[x];
			out_z++;
		}
		for(int x = right_pos + right_width; x < length; x++)
		{
			out_average_right += power_array[x];
			out_z_right++;
			out_average += power_array[x];
			out_z++;
		}
		out_average /= out_z;
		out_average_left /= out_z_left;
		out_average_right /= out_z_right;
		for(int x = 0; x < left_pos - left_width; x++)
		{
			if(power_array[x] < out_average_left)
			{
				out_low_average_left += power_array[x];
				out_low_z_left++;
			}
			if(power_array[x] < out_average)
			{
				out_low_average += power_array[x];
				out_low_z++;
			}
		}
		for(int x = right_pos + right_width; x < length; x++)
		{
			if(power_array[x] < out_average_right)
			{
				out_low_average_right += power_array[x];
				out_low_z_right++;
			}
			if(power_array[x] < out_average)
			{
				out_low_average += power_array[x];
				out_low_z++;
			}
		}
		out_low_average /= out_low_z;
		out_low_average_left /= out_low_z_left;
		out_low_average_right /= out_low_z_right;
		
//		out_average = out_average_left;
//		if(out_average_right > out_average) out_average = out_average_right;
//		out_low_average = out_low_average_left;
//		if(out_low_average_right > out_low_average) out_low_average = out_low_average_right;
		
		float left_power = 0;
		float left_power_high = 0;
		float left_power_highZ = 0;
		float right_power = 0;
		float right_power_high = 0;
		float right_power_highZ = 0;

		if(!single_peak)
		{
			for(int x = left_pos - left_width; x < left_pos + left_width; x++)
				left_power += power_array[x];
			left_power /= (float)(2*left_width);
			for(int x = left_pos - left_width; x < left_pos + left_width; x++)
				if(power_array[x] > left_power)
				{
					left_power_high += power_array[x];
					left_power_highZ++;
				}
			left_power_high /= left_power_highZ;

			for(int x = right_pos - right_width; x < right_pos + right_width; x++)
				right_power += power_array[x];
			right_power /= (float)(2*right_width);
			for(int x = right_pos - right_width; x < right_pos + right_width; x++)
				if(power_array[x] > right_power)
				{
					right_power_high += power_array[x];
					right_power_highZ++;
				}
			right_power_high /= right_power_highZ;
		}
		
		float center_power = 0;
		for(int x = center_pos - center_width; x < center_pos + center_width; x++)
			center_power += power_array[x];
		center_power /= (float)(2*center_width);
		float center_power_high = 0;
		float center_power_highZ = 0;
		for(int x = center_pos - center_width; x < center_pos + center_width; x++)
			if(power_array[x] > center_power)
			{
				center_power_high += power_array[x];
				center_power_highZ++;
			}
		center_power_high /= center_power_highZ;

		float center_power_high_high = 0;
		float center_power_high_highZ = 0;
		for(int x = center_pos - center_width; x < center_pos + center_width; x++)
			if(power_array[x] > center_power_high)
			{
				center_power_high_high += power_array[x];
				center_power_high_highZ++;
			}
		center_power_high_high /= center_power_high_highZ;
		
		
		float center_to_background = center_power_high - out_low_average;
		float left_to_background = left_power_high - out_low_average;
		float right_to_background = right_power_high - out_low_average;

//		float center_to_left = (center_power_high - left_power_high) / center_to_background;
//		float center_to_right = (center_power_high - right_power_high) / center_to_background;
		
		float detected_score = 0;
				
		float rel_left = left_relative_power - (left_to_background / center_to_background);
		float rel_right = right_relative_power - (right_to_background / center_to_background);
		rel_left = 1.0 - fabs(rel_left);
		rel_right = 1.0 - fabs(rel_right);
		if(rel_left < 0.01) rel_left = 0.01;
		if(rel_right < 0.01) rel_right = 0.01;
//		detected_score = center_to_background * rel_left * rel_right;
		if(single_peak) detected_score = center_to_background;
		else detected_score = center_to_background * rel_left * rel_right;
//		printf("ds: %g, cb: %g rl %g rr %g\n", detected_score, center_to_background, rel_left, rel_right);

		float freq_penalty_mult = 1.0;
		if(center_freq < standard_min_frequency_MHz*1000000.0)
			freq_penalty_mult = center_freq / (standard_min_frequency_MHz*1000000.0);
		if(center_freq > standard_max_frequency_MHz*1000000.0)
			freq_penalty_mult = standard_max_frequency_MHz*1000000.0 / center_freq;

		detected_score *= freq_penalty_mult;
//		float detected_normalized = freq_penalty_mult*detected_score / (20.0 + detected_score);
		
		*res_power = center_power;
		
		float cur_level = power_array[left_pos];
		int left_edge = left_pos;
		for(int x = left_pos; x > 0; x--)
		{
			cur_level = cur_level * 0.9 + 0.1*power_array[x];
			left_edge = x;
			if(cur_level < (left_power + out_average) * 0.5)
				break;
		}
		int right_edge = right_pos;
		cur_level = power_array[right_pos];
		for(int x = right_pos; x < length; x++)
		{
			cur_level = cur_level * 0.9 + 0.1*power_array[x];
			right_edge = x;
			if(cur_level < (right_power + out_average) * 0.5)
				break;
		}
		float left_freq_bw = frequency_start_hz + frequency_step_hz * left_edge;
		float right_freq_bw = frequency_start_hz + frequency_step_hz * right_edge;
		
		*res_bw = right_freq_bw - left_freq_bw;
		return detected_score;
	}
}sSignalDetector_old;

