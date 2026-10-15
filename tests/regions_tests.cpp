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
 // Bounded interpolation oracle: moving square, two raw anchors, missing current.
 AnalysisImage image;image.width=image.height=64;image.rgba.assign(64*64*4,128);
 ColorRegion seed{.3,.3,.6,.6,1,{}};
 RegionFrame l{-1,image,{seed}},c{0,image,{}},rr{1,image,{seed}};
 auto restored=recover_color_regions({rr,c,l});check(restored.size()==1&&restored[0].detail==0&&restored[0].holes.empty(),"Two-sided coarse interpolation failed");
 check(restored[0].x0==seed.x0&&restored[0].x1==seed.x1,"Stationary position changed");
 check(recover_color_regions({c,l}).empty(),"One-sided extrapolation accepted");
 auto changed=c;changed.image.rgba.assign(64*64*4,0);check(recover_color_regions({l,changed,rr}).empty(),"Cut/appearance change accepted");
 auto local=c;for(int y=19;y<38;++y)for(int x=19;x<38;++x)for(int ch=0;ch<3;++ch)local.image.rgba[(y*64+x)*4+ch]=255;
 check(recover_color_regions({l,local,rr}).empty(),"Local occluder accepted");
 auto ambiguous=l;ambiguous.regions.push_back(seed);check(recover_color_regions({ambiguous,c,rr}).empty(),"Ambiguous identity accepted");
 auto occupied=c;occupied.regions={seed};check(recover_color_regions({l,occupied,rr}).size()==1,"Current detection duplicated");
 auto farleft=l;farleft.offset=-2;RegionFrame missing{-1,image,{}};
 check(recover_color_regions({farleft,missing,c,rr}).size()==1,"Two-frame gap rejected");
 auto competitor=missing;competitor.regions={{.59,.3,.65,.6,0,{}}};
 check(recover_color_regions({farleft,competitor,c,rr}).empty(),"Search evaded intermediate competitor");
 auto farright=rr;farright.offset=2;RegionFrame missingright{1,image,{}};
 check(recover_color_regions({farleft,missing,c,missingright,farright}).empty(),"Long gap accepted");
 check(recover_color_regions({farleft,c,rr}).empty(),"Unobserved intervening frame ignored");
 bool invalid=false;try{recover_color_regions({l,c,c,rr});}catch(const std::invalid_argument&){invalid=true;}check(invalid,"Duplicate frame accepted");
 auto moving=rr;moving.regions[0].x0+=.06;moving.regions[0].x1+=.06;
 auto midpoint=recover_color_regions({l,c,moving});check(midpoint.size()==1&&std::abs(midpoint[0].x0-.33)<1e-12,"Position not time interpolated");
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
