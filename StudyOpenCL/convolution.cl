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

void kernel gaussian_filter(__read_only image2d_t src, __write_only image2d_t dst, sampler_t sampler, int width, int height){
	float kernel_weight[9] = {1.0f, 2.0f, 1.0f, 2.0f,4.0f,2.0f,1.0f,2.0f,1.0f};
	int2 start_image_coord = (int2)(get_global_id(0) -1 , get_global_id(1) - 1);
	int2 end_image_coord = (int2)(get_global_id(0)+1,get_global_id(1)+1);
	int2 out_image_coord = (int2)(get_global_id(0),get_global_id(1));

	if(out_image_coord.x < width && out_image_coord.y < height){
		int weight = 0;
		float4 out_color = (float4)(0.0f,0.0f,0.0f,0.0f);
		for(int y=start_image_coord.y; y<=end_image_coord.y; y++){
			for(int x = start_image_coord.x; x<=end_image_coord.x; x++){
				out_color += read_imagef(src,sampler,(int2)(x,y)) * (kernel_weight[weight] / 16.0f);
				weight +=1;
			}
		}
		write_imagef(dst,out_image_coord,out_color);
	}

	
}