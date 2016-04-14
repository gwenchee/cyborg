#include "cyclus_origen_interface.h"
#include "error.h"
#include "Origen/Core/dc/ConcentrationConverter.h"
#include "Origen/Core/fn/io.h"
#include "Origen/Core/fn/interp.h"
#include "Origen/Core/io/LibraryIO.h"
#include "ScaleUtils/IO/DB.h"
#include "ScaleUtils/IO/Utils.h"

namespace OrigenInterface {

void cyclus2origen::set_lib_names(const std::vector<std::string> &lib_names){
  b_lib_names.clear();
  b_lib_names.resize(lib_names.size());
  b_lib_names=lib_names;
}

void cyclus2origen::set_lib_path(const std::string lib_path){
  using cyclus::IOError;
  auto dr = opendir(lib_path.c_str());
  if(dr==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_lib_path(" << __LINE__ << ") : Directory provided is not a directory!" << std::endl;
    throw IOError(ss.str());
  }
  closedir(dr);
  b_lib_path=lib_path;
}

void cyclus2origen::add_lib_names(const std::vector<std::string> &lib_names){
  for(auto lib : lib_names) b_lib_names.push_back(lib);
}

void cyclus2origen::remove_lib_names(const std::vector<std::string> &lib_names){
  for(size_t i = 0; i < lib_names.size(); i++){
    for(size_t j = 0; j < b_lib_names.size(); j++){
      if(b_lib_names[j]==lib_names[i]){
        b_lib_names.erase(b_lib_names.begin()+j);
        break;
      }
    }
  }
}

void cyclus2origen::list_lib_names() const{
  for(auto lib : b_lib_names) std::cout << "Library name: " << lib << std::endl;
}

void cyclus2origen::get_lib_names(std::vector<std::string> &lib_names) const{
  using cyclus::StateError;
  if(!lib_names.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_lib_names(" << __LINE__ << ") : Return vector for lib_names not empty upon function call!" << std::endl;
    throw StateError(ss.str());
  }
  lib_names=b_lib_names;
}

void cyclus2origen::set_id_tag(const std::string idname, const std::string idvalue){
  if(b_tm==NULL) b_tm = Origen::SP_TagManager(new Origen::TagManager());
  b_tm->setIdTag(idname,idvalue);
}

void cyclus2origen::set_id_tags(const std::map<std::string,std::string> &tags){
  for(auto tag : tags){
    this->set_id_tag(tag.first,tag.second);
  }
}

void cyclus2origen::remove_id_tag(const std::string idname){
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::remove_id_tag(" << __LINE__ << ") : No tag manager found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }
  if(!b_tm->hasTag(idname)){
    std::stringstream ss;
    ss << "Cyborg::reactor::remove_id_tag(" << __LINE__ << ") : Tag manager does not have a tag with name = " << idname << "!" << std::endl;
    throw StateError(ss.str());
  }
  b_tm->deleteTag(idname);
}

void cyclus2origen::list_id_tags() const{
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::list_id_tags(" << __LINE__ << ") : No tag manager found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_tm->listIdTags().size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::list_id_tags(" << __LINE__ << ") : No ID tags found on this interface object!  Use set_id_tags()." << std::endl;
    throw StateError(ss.str());
  }
  for(auto tags : b_tm->listIdTags()){
    std::cout << "Tag name: " << tags << ", value: " << b_tm->getIdTag(tags) << "." << std::endl;
  }
}

void cyclus2origen::get_id_tags(std::vector<std::string> &names, std::vector<std::string> &values) const{
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_id_tags(" << __LINE__ << ") : No tag manager found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }
  if(!names.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_id_tags(" << __LINE__ << ") : Return vector for ID tag names not emtpy upon function call!" << std::endl;
    throw StateError(ss.str());
  }
  if(!values.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_id_tags(" << __LINE__ << ") : Return vector for ID tag values not empty upon function call!" << std::endl;
    throw StateError(ss.str());
  }
  for(auto tag : b_tm->listIdTags()){
    names.push_back(tag);
    values.push_back(b_tm->getIdTag(tag));
  }
}

void cyclus2origen::set_materials_with_masses(std::vector<int> &ids, const std::vector<double> &masses){
  using cyclus::StateError;
  using cyclus::ValueError;
  if(ids.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials_with_masses(" << __LINE__ << ") : No IDs provided in ID vector!" << std::endl;
    throw StateError(ss.str());
  }
  if(masses.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials_with_masses(" << __LINE__ << ") : No masses provided in the masses vector!" << std::endl;
    throw StateError(ss.str());
  }
  if(masses.size()!=ids.size()){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials_with_masses(" << __LINE__ << ") : Size mismatch between ID and mass vectors!" << std::endl;
    throw ValueError(ss.str());
  }
  std::vector<double> concs;
  Origen::ConcentrationConverter cv;
  for(size_t i = 0; i < ids.size(); i++){
    concs.push_back(cv.convert_to(Origen::ConcentrationUnit::CM_2_BARN,ids[i],Origen::ConcentrationUnit::KILOGRAMS,masses[i],b_vol));
  }
  this->set_materials(ids,concs);
}

void cyclus2origen::set_materials(const std::vector<int> &ids, const std::vector<double> &concs){
  using cyclus::StateError;
  using cyclus::ValueError;
  if(b_lib==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : No library found on this interface object!  Use interpolate() first." << std::endl;
    throw StateError(ss.str());
  }
  if(b_vol<=0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : Volume should be implicitly set...." << std::endl;
    throw ValueError(ss.str());
  }
  if(ids.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : No IDs provided in ID vector!" << std::endl;
    throw StateError(ss.str());
  }
  if(concs.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : No concentrations were provided in the concentrations vector!" << std::endl;
    throw StateError(ss.str());
  }
  if(concs.size()!=ids.size()){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : Size mismatch between ID and concentrations vectors!" << std::endl;
    throw ValueError(ss.str());
  }
  std::string name = "cyclus_";
  if(std::string::npos==b_interp_name.find(name)){
    name.append(b_interp_name);
  }
  int id = 1001;
  std::vector<int> tmp_ids;
  tmp_ids.assign(ids.begin(),ids.end());
  std::vector<double> tmp_concs;
  tmp_concs.assign(concs.begin(),concs.end());
  for(size_t i = 0; i < ids.size(); i++){
    if(b_init_ids.size() <= i || b_init_ids.at(i)!=ids.at(i)){
      b_init_ids.clear();
      b_init_concs.clear();
      for(size_t i = 0; i < tmp_ids.size(); i++){
        if(ScaleData::Utils::is_valid_pizzzaaa(tmp_ids[i])) continue;
        if(ScaleData::Utils::is_valid_zzzaaai(tmp_ids[i])) tmp_ids[i] = ScaleData::Utils::zzzaaai_to_pizzzaaa(tmp_ids[i]);
        if(!ScaleData::Utils::is_valid_pizzzaaa(tmp_ids[i])){
          std::stringstream ss;
          ss << "Cyborg::reactor::set_materials(" << __LINE__ << ") : Unrecognizeable nuclide ID name! Use zzzaaai or pizzzaaa format." << std::endl;
          throw ValueError(ss.str());
        }
      }
      b_init_ids.assign(tmp_ids.begin(),tmp_ids.end());
      b_init_concs.assign(tmp_concs.begin(),tmp_concs.end());
      break;
    }else if(b_init_concs.at(i)!=concs.at(i)){
      b_init_concs.at(i)=concs.at(i);
    }
  }
  Origen::SP_Material mat = Origen::SP_Material(new Origen::Material(b_lib,name,id,b_vol));
  mat->set_numden_bos(tmp_concs,tmp_ids,b_vol);
  b_mat = mat;
}

void cyclus2origen::delete_material(){
  if(b_mat!=nullptr) b_mat->clear();
}

void cyclus2origen::set_mat_units(const std::string mat_units){
  b_concs_units = Origen::convertStringToConcUnit(mat_units);
}

void cyclus2origen::set_time_units(const char* time_units){
  b_time_units = Origen::Time::units(time_units);
}

void cyclus2origen::add_time_step(const double time){
  using cyclus::StateError;
  if(time<0){
    std::stringstream ss;
    ss << "Cyborg::reactor::add_time_step(" << __LINE__ << ") : Time value provided is non-physical (<0)!" << std::endl;
    throw StateError(ss.str());
  }
  b_times.push_back(time);
}

void cyclus2origen::set_fluxes(const std::vector<double> &fluxes){
  using cyclus::StateError;
  if(fluxes.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_flux(" << __LINE__ << ") : Vector of fluxes provided is empty!" << std::endl;
    throw StateError(ss.str());
  }
  b_fluxes = fluxes;
}

void cyclus2origen::add_flux(const double flux){
  using cyclus::StateError;
  if(flux<0){
    std::stringstream ss;
    ss << "Cyborg::reactor::add_flux(" << __LINE__ << ") : Flux value provided is non-physical (<0)!" << std::endl;
    throw StateError(ss.str());
  }
  b_fluxes.push_back(flux);
}

void cyclus2origen::delete_fluxes(){
  if(b_fluxes.size()>0) b_fluxes.clear();
}

void cyclus2origen::set_powers(const std::vector<double> &powers){
  using cyclus::StateError;
  if(powers.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::set_powers(" << __LINE__ << ") : Vector of powers provided is empty!" << std::endl;
    throw StateError(ss.str());
  }
  b_powers = powers;
}

void cyclus2origen::add_power(const double power){
  using cyclus::StateError;
  if(power<0){
    std::stringstream ss;
    ss << "Cyborg::reactor::add_power(" << __LINE__ << ") : Power provided is non-physical (<0)!" << std::endl;
    throw StateError(ss.str());
  }
  b_powers.push_back(power);
}

void cyclus2origen::delete_powers(){
  if(b_powers.size()>0) b_powers.clear();
}

void cyclus2origen::add_parameter(const std::string name, const double value){
  if(b_tm==NULL) b_tm = Origen::SP_TagManager(new Origen::TagManager());
  b_tm->setInterpTag(name,value);
}

void cyclus2origen::set_parameters(const std::map<std::string,double> &params){
  for(auto param : params){
    this->add_parameter(param.first,param.second);
  }
}

void cyclus2origen::remove_parameter(const std::string name){
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::remove_parameter(" << __LINE__ << ") : No tag manager found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }
  if(!b_tm->hasTag(name)){
    std::stringstream ss;
    ss << "Cyborg::reactor::remove_parameter(" << __LINE__ << ") : No tag with name " << name << " found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }

  b_tm->deleteTag(name);
}

void cyclus2origen::list_parameters() const{
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::list_parameters(" << __LINE__ << ") : No tag manager found on this interface object!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_tm->listInterpTags().size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::list_parameters(" << __LINE__ << ") : No parameters found on this tag manager!" << std::endl;
    throw StateError(ss.str());
  }
  for(auto tag : b_tm->listInterpTags()){
    std::cout << "Interp tag name: " << tag << ", value: " << b_tm->getInterpTag(tag) << "." << std::endl;
  }
}

void cyclus2origen::get_parameters(std::vector<std::string> &names, std::vector<double> &values) const{
  using cyclus::StateError;
  if(!names.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_parameters(" << __LINE__ << ") : Return vector for names not empty upon function call!" << std::endl;
    throw StateError(ss.str());
  }
  if(!values.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_parameters(" << __LINE__ << ") : Return vector for values not empty upon function call!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_parameters(" << __LINE__ << ") : No tag manager present with parameters!" << std::endl;
    throw StateError(ss.str());
  }
  for(auto tag : b_tm->listInterpTags()){
    names.push_back(tag);
    values.push_back(b_tm->getInterpTag(tag));
  }
}

//void set_solver(const int svlr=0){
//  Defaulting to CRAM.  Should be this easy to swap in
//  matrex if desired, but not including it for now.
//
//  if(slvr==1){
//    b_slv=Origen::Solver_matrex();
//  } else {
//    b_slv=Origen::Solver_cram();
//  }
//}

void cyclus2origen::interpolate() {
  using cyclus::ValueError;
  using cyclus::StateError;
  if(b_tm==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No tag manager found!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_tm->listIdTags().size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No ID tags found!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_lib_names.size()==0 && b_lib_path.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No library names or path specified!" << std::endl;
    throw cyclus::ValueError(ss.str());
  }

  if(b_lib_names.size()==0){
    struct dirent *drnt;
    auto dr = opendir(b_lib_path.c_str());
    std::string midstring = "";
    if(&(b_lib_path.back()) != "/") midstring = "/";
    while(true){
      drnt=readdir(dr);
      if(!drnt) break;
      std::string lib_name (drnt->d_name);
      if(lib_name=="." || lib_name=="..") continue;
      lib_name = b_lib_path + midstring + lib_name;
      struct stat buffer;
      if(stat(lib_name.c_str(), &buffer) != 0){
        std::cout << lib_name << " doesn't exist!" << std::endl;
      }else{
        b_lib_names.push_back(lib_name);
      }
    }
    closedir(dr);
  }

  // Bail if no libraries specified
  if(b_lib_names.size() == 0) {
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No libraries specified or found!" << std::endl;
    throw ValueError(ss.str());
  }

  std::vector<Origen::SP_TagManager> tms = Origen::collectLibraries(b_lib_names);
  std::vector<Origen::TagManager> tagman;
  for(auto& tm : tms) tagman.push_back(*tm);

  // Bail if no libraries found
  if(tagman.size() == 0) {
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No libraries found that have tag managers!" << std::endl;
    throw ValueError(ss.str());
  }

  tagman = Origen::selectLibraries(tagman,*b_tm);

  if(tagman.size() == 0){
    std::stringstream ss;
    ss << "Cyborg::reactor::interpolate(" << __LINE__ << ") : No libraries found that match specified ID tags!" << std::endl;
    throw ValueError(ss.str());
  }

  b_lib = Origen::interpLibraryND(tagman,*b_tm);
  b_interp_name = (b_lib->scp_tag_manager())->getIdTag("Filename");
}

void cyclus2origen::solve(){
  using cyclus::StateError;
  using cyclus::ValueError;
  if(b_mat==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : No material object found on this interface object!  Use set_materials() or set_materials_with_masses()." << std::endl;
    throw StateError(ss.str());
  }
  if(b_mat->library()==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : No library object found on the material object on this interface object.!" << std::endl;
    throw StateError(ss.str());
  }
  if(b_powers.size()==0 && b_fluxes.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : No powers or fluxes found on this interface object!  Use set_fluxes or set_powers." << std::endl;
    throw StateError(ss.str());
  }
  if(b_powers.size()!=(b_times.size()-1) && b_powers.size()!=0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : Powers vector must be exactly 1 element shorter than times vector!" << std::endl \
       << "Power vector size is " << b_powers.size() << " and times vector size is " << b_times.size() << "." << std::endl;
    throw ValueError(ss.str());
  }
  if(b_fluxes.size()!=(b_times.size()-1) && b_fluxes.size()!=0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : Fluxes vector must be exactly 1 element shorter than times vector!" << std::endl \
       << "Fluxes vector size is " << b_fluxes.size() << " and times vector size is " << b_times.size() << "." << std::endl;
    throw ValueError(ss.str());
  }
  prob_spec_lib(b_lib,b_times,b_fluxes,b_powers);
  set_materials(b_init_ids,b_init_concs);
  Origen::SP_Solver solver;
  ScaleUtils::IO::DB db;
  db.set<std::string>("solver","cram");
  solver = Origen::SolverSelector::get_solver(db);
  b_mat->set_solver(solver);
  size_t num_steps = b_powers.size()>b_fluxes.size() ? b_powers.size() : b_fluxes.size();
  for(size_t i = 0; i < num_steps; i++){
    b_mat->add_step(b_times[i+1]-b_times[i]);
    b_mat->set_transition_matrix(b_mat->library()->newsp_transition_matrix_at(i));
    if(b_fluxes.size()==0){
      b_mat->set_flux(b_powers[i]/b_mat->power_factor_bos());
    }else if(b_powers.size()==0){
      b_mat->set_flux(b_fluxes[i]);
    }
    solver->set_transition_matrix( &*b_mat->transition_matrix() );
    Origen::SP_Vec_Dbl n0 = b_mat->amount_bos();
    Origen::SP_Vec_Dbl n1 = b_mat->amount_eos();
    solver->solve(*n0,b_mat->flux(),b_mat->dt(),&*n1);
    solver->clear();
  }
}

void cyclus2origen::solve(std::vector<double>& times, std::vector<double>& fluxes, std::vector<double>& powers){
  using cyclus::StateError;
  using cyclus::ValueError;
  if(b_mat==NULL){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : No material object found on this interface object!  Use set_materials() or set_materials_with_masses()." << std::endl;
    throw StateError(ss.str());
  }
  if(powers.size()==0 && fluxes.size()==0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : No powers or fluxes found on this interface object!  Use set_fluxes or set_powers." << std::endl;
    throw StateError(ss.str());
  }
  if(powers.size()!=(times.size()-1) && powers.size()!=0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : Powers vector must be exactly 1 element shorter than times vector!" << std::endl \
       << "Power vector size is " << powers.size() << " and times vector size is " << times.size() << "." << std::endl;
    throw ValueError(ss.str());
  }
  if(fluxes.size()!=(times.size()-1) && fluxes.size()!=0){
    std::stringstream ss;
    ss << "Cyborg::reactor::solve(" << __LINE__ << ") : Fluxes vector must be exactly 1 element shorter than times vector!" << std::endl \
       << "Fluxes vector size is " << fluxes.size() << " and times vector size is " << times.size() << "." << std::endl;
    throw ValueError(ss.str());
  }
  Origen::SP_Solver solver;
  ScaleUtils::IO::DB db;
  db.set<std::string>("solver","cram");
  solver = Origen::SolverSelector::get_solver(db);
  b_mat->set_solver(solver);
  prob_spec_lib(b_lib,times,fluxes,powers);
  size_t num_steps = powers.size()>fluxes.size() ? powers.size() : fluxes.size();
  for(size_t i = 0; i < num_steps; i++){
    b_mat->allocate_step();
    b_mat->set_transition_matrix(b_mat->library()->newsp_transition_matrix_at(0));
    if(fluxes.size()==0){
      b_mat->set_flux(powers[i]/b_mat->power_factor_bos());
    }else if(powers.size()==0){
      b_mat->set_flux(fluxes[i]);
    }
    b_mat->set_dt(times[i+1]-times[i]);

    solver->set_transition_matrix( &*b_mat->transition_matrix() );
    Origen::SP_Vec_Dbl n0 = b_mat->amount_bos();
    Origen::SP_Vec_Dbl n1 = b_mat->amount_eos();
    solver->solve(*n0,b_mat->flux(),b_mat->dt(),&*n1);
    solver->clear();
  }
}

void cyclus2origen::get_concentrations(std::vector<std::vector<double>> &concs_out) const{
  for(size_t i = 0; i < b_mat->ntimes(); i++){
    Origen::SP_DoubleList vals = b_mat->amount_at(i);
    std::vector<double> vals_vec;
    for(size_t j = 0; j < vals->size(); j++){
      vals_vec.push_back(vals->at(j));
    }
    concs_out.push_back(vals_vec);
  }
}

void cyclus2origen::get_concentrations_at(int p, std::vector<double> &concs_out) const{
  using cyclus::ValueError;
  if(p<0 || p>=b_mat->ntimes()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_concentrations_at(" << __LINE__ << ") : Step requested " << p << " falls outside the bounds [0," << b_mat->ntimes() << "]!" << std::endl;
    throw ValueError(ss.str());
  }
  Origen::SP_DoubleList vals = b_mat->amount_at(p);
  for(size_t i = 0; i < vals->size(); i++){
    concs_out.push_back(vals->at(i));
  }
}

void cyclus2origen::get_concentrations_final(std::vector<double> &concs_out) const{
  Origen::SP_DoubleList vals = b_mat->amount_at(b_mat->ntimes()-1);
  for(size_t i = 0; i < vals->size(); i++){
    concs_out.push_back(vals->at(i));
  }
}

void cyclus2origen::get_masses(std::vector<std::vector<double> > &masses_out) const{
  using cyclus::ValueError;
  if(!masses_out.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_masses(" << __LINE__ << ") : Return vector for masses is not empty upon function call!" << std::endl;
    throw ValueError(ss.str());
  }
  std::vector<std::vector<double> > concs;
  this->get_concentrations(concs);
  std::vector<int> ids;
  this->get_ids(ids);
  if(ids.size()!=concs[0].size()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_masses(" << __LINE__ << ") : Size mismatch between returned ID vector size and returned concentrations vector size!" << std::endl;
    throw ValueError(ss.str());
  }
  Origen::ConcentrationConverter cv;
  for(size_t i = 0; i < concs.size(); i++){
    std::vector<double> tmp;
    for(size_t j = 0; j < concs[0].size(); j++){
      tmp.push_back(cv.convert_to(Origen::ConcentrationUnit::KILOGRAMS,ids[j],Origen::ConcentrationUnit::CM_2_BARN,concs[i][j],b_vol));
    }
    masses_out.push_back(tmp);
  }
}

void cyclus2origen::get_masses_at(int p, std::vector<double> &masses_out) const{
  using cyclus::ValueError;
  if(!masses_out.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_masses_at(" << __LINE__ << ") : Masses return vector is not empty upon function call!" << std::endl;
    throw ValueError(ss.str());
  }
  std::vector<double> concs;
  this->get_concentrations_at(p,concs);
  std::vector<int> ids;
  this->get_ids(ids);
  Origen::ConcentrationConverter cv;
  for(size_t i = 0; i < concs.size(); i++){
    masses_out.push_back(cv.convert_to(Origen::ConcentrationUnit::KILOGRAMS,ids[i],Origen::ConcentrationUnit::CM_2_BARN,concs[i],b_vol));
  }
}

void cyclus2origen::get_masses_final(std::vector<double> &masses_out) const{
  using cyclus::ValueError;
  if(!masses_out.empty()){
    std::stringstream ss;
    ss << "Cyborg::reactor::get_masses_final(" << __LINE__ << ") : Masses return vector is not empty upon function call!" << std::endl;
    throw ValueError(ss.str());
  }
  std::vector<double> concs;
  this->get_concentrations_final(concs);
  std::vector<int> ids;
  this->get_ids(ids);
  Origen::ConcentrationConverter cv;
  for(size_t i = 0; i < concs.size(); i++){
    masses_out.push_back(cv.convert_to(Origen::ConcentrationUnit::KILOGRAMS,ids[i],Origen::ConcentrationUnit::CM_2_BARN,concs[i],b_vol));
  }
}

void cyclus2origen::get_ids(std::vector<int> &ids_out) const{
  ids_out = *(b_mat->sizzzaaa_list());
}

void cyclus2origen::get_ids_zzzaaai(std::vector<int> &ids_out) const{
  for(auto id : *(b_mat->sizzzaaa_list())){
    ids_out.push_back(ScaleData::Utils::pizzzaaa_to_zzzaaai(id));
  }
}

void cyclus2origen::prob_spec_lib(Origen::SP_Library lib,std::vector<double> &times,std::vector<double> &fluxes,std::vector<double> &powers){
  if(b_time_units!=Origen::Time::DAYS){
    for(auto& time : times) time /= Origen::Time::factor(b_time_units,Origen::Time::DAYS);
  }
  if(fluxes.size()>0&&powers.size()==0){
    for(auto& flux : fluxes) powers.push_back(flux*b_mat->power_factor_bos());
  }else if(fluxes.size()>0&&powers.size()>0){
    std::stringstream ss;
    ss << "Cyborg::reactor::prob_spec_lib(" << __LINE__ << ") : Both the fluxes and powers vectors have values! Choose one!" << std::endl;
    throw cyclus::ValueError(ss.str());
  }
  std::vector<double> burnups;
  if(times.size()!=powers.size()+1){
    std::stringstream ss;
    ss << "Cyborg::reactor::prob_spec_lib(" << __LINE__ << ") : Powers or fluxes vectors not exactly 1 element shorter than times vector!" << std::endl \
       << "Powers or fluxes vector has " << powers.size() << " elements and times vector has " << times.size() << " elements." << std::endl;
    throw cyclus::ValueError(ss.str());
  }
  for(size_t i = 0; i < powers.size(); i++){
    // 1e3 factor arises from converting powers from watts to megawatts and mass from g to MT.
    burnups.push_back(1e3*powers[i]*(times[i+1]-times[i])/b_mat->initial_hm_mass());
  }
  if(burnups.size()!=powers.size()){
    std::stringstream ss;
    ss << "Cyborg::reactor::prob_spec_lib(" << __LINE__ << ") : Calculated burnup vector does not have same size as provided powers vector!" << std::endl;
    throw cyclus::StateError(ss.str());
  }
  b_lib = lib->interpolate_Interp1D(burnups);
}

}//namespace