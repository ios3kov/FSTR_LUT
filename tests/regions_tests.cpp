#include "fstr/regions.hpp"
#include "fstr/color.hpp"
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
int main(){try{
 using namespace fstr;ColorRegion r{.2,.2,.8,.8,0,{}};std::vector<ColorRegion> rs{r};
 check(color_region_weight(.5,.5,1000,rs)==.65,"Coarse center");
 check(color_region_weight(.1,.5,1000,rs)==0,"Outside protection");
 double previous=.65;for(int i=0;i<=1000;++i){double v=color_region_weight(.5+.3*i/1000.,.5,1000,rs);check(v<=previous+1e-12&&previous-v<.01,"Outer edge discontinuity");previous=v;}
 rs[0].detail=1;rs[0].holes={{{.4,.4},{.6,.4},{.6,.6},{.4,.6}}};
 previous=color_region_weight(.38,.5,1000,rs);for(int i=1;i<=1000;++i){double v=color_region_weight(.38+.12*i/1000.,.5,1000,rs);check(std::abs(v-previous)<.01,"Internal edge discontinuity");previous=v;}
 check(previous>=.325,"Detailed hole punctures coarse base");
 auto blurred=rs;blurred[0].detail=0;auto stabilized=stabilize_color_regions(blurred,rs,rs);check(stabilized[0].detail==0,"Sharp neighbors restore blur detail");
 check(stabilize_color_regions({},rs,rs).empty(),"Unconfirmed native gap invented");
 // Constant red LUT: full chroma protection retains its luminance change only.
 std::istringstream cube("LUT_3D_SIZE 2\n1 0 0\n1 0 0\n1 0 0\n1 0 0\n1 0 0\n1 0 0\n1 0 0\n1 0 0\n");
 ColorSettings settings;settings.look=std::make_shared<LUT>(LUT::parse(cube));settings.look_amount=1;
 ColorProcessor processor(settings,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight);
 std::array<float,4> pixel{.2f,.2f,.2f,1};auto off=processor.apply(pixel),on=processor.apply(pixel,1),half=processor.apply(pixel,.5);
 check(off[0]==1&&off[1]==0&&off[2]==0,"Off changed LUT");
 for(int c=0;c<3;++c){check(std::abs(on[c]-.2126)<1e-6,"Protection luminance oracle");check(std::abs(half[c]-(off[c]+on[c])*.5)<1e-6,"Protection linear mix");}
 check(on[3]==pixel[3],"Alpha changed");
 std::cout<<"PASS native feather, blur detail, absent regions, chroma/luminance contracts\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
