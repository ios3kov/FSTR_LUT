#include "fstr/color.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

// Authored linear reference fixture; no external footage/profile inference.
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::invalid_argument("Usage: fstr_color_fixture output.ppm output.csv");
        std::ofstream image(argv[1],std::ios::binary),data(argv[2]);
        if(!image||!data)throw std::runtime_error("Cannot create fixture outputs");
        constexpr int width=512,band=72,rows=4;
        image<<"P6\n"<<width*2<<" "<<band*rows<<"\n255\n";
        data<<"row,x,input_r,input_g,input_b,output_r,output_g,output_b\n"<<std::setprecision(17);
        fstr::GradeSettings settings[rows]={{1,1,1},{0,1.3,1},{0,1,.5},{.25,1.2,1.1}};
        for(int row=0;row<rows;++row) {
            for(int y=0;y<band;++y)for(int col=0;col<2;++col)for(int x=0;x<width;++x) {
                const double t=double(x)/(width-1);
                fstr::RGB input;
                if(y<band/2)input={-.05+1.25*t,-.05+1.25*t,-.05+1.25*t};
                else input={.03+.65*t,.03+.3*t,.6-.4*t};
                const auto output=fstr::grade_linear(input,settings[row]);
                const auto displayed=col==0?input:output;
                for(double v:{displayed.r,displayed.g,displayed.b}) {
                    // PPM is a clamped SDR preview only. CSV retains negative/above-one linear results.
                    const auto encoded=fstr::linear_to_srgb(v);
                    image.put(static_cast<char>(static_cast<unsigned char>(std::clamp(encoded,0.0,1.0)*255+.5)));
                }
                if(y==0&&col==0)data<<row<<","<<x<<","<<input.r<<","<<input.g<<","<<input.b<<","<<output.r<<","<<output.g<<","<<output.b<<"\n";
            }
        }
        image.flush();data.flush();
        if(!image||!data)throw std::runtime_error("Incomplete fixture write");
        std::cout<<"PASS: authored linear Rec709 fixture; SDR preview clamps only, CSV float values preserved\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
