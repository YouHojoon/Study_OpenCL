void kernel convolve(global const uint* input, constant const uint* mask, global uint* output, const int input_width, const int mask_width){
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	uint sum = 0;
	for(int r = 0; r< mask_width; r++){
		const int idx = (y+r) * input_width + x;
		for(int c=0; c<mask_width; c++){
			sum += mask[(r*mask_width) + c] * input[idx + c];
		}
	}

	output[y*get_global_size(0) + x] = sum;
}
