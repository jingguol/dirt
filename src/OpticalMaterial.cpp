#include <cmath>
#include <cstring>
#include <stdio.h>
#include <PolynomialOptics/OpticalMaterial.hh>

OpticalMaterial::OpticalMaterial(const char *query, float nd, float vd, bool print_match)
{
    init(query, nd, vd, print_match);
}

void OpticalMaterial::init(const char *query, float nd, float vd, bool print_match)
{
    // init by mat name, fail over to ior/abbe
    if(!init(query, print_match))
        init(nd, vd, print_match);
}

OpticalMaterial::OpticalMaterial(float nd, float vd, bool print_match)
{
    init(nd, vd, print_match);
}

bool OpticalMaterial::init(float nd, float vd, bool print_match)
{
  float bestDistance 
    = __sqr(glass_database[0].nd-nd) +  1e-4 * __sqr(glass_database[0].vd-vd);
    
  GlassDef bestMaterial = glass_database[0];

  for (int i = 1; i < NUM_MATERIALS; ++i) {
    float newDistance
      = __sqr(glass_database[i].nd-nd) +  1e-4 * __sqr(glass_database[i].vd-vd);
      
    if (newDistance < bestDistance) {
      bestDistance = newDistance;
      bestMaterial = glass_database[i];
    }

  }
    
  // Copy best match over to coefficients:    
  model = bestMaterial.model;
  coeff[0] = bestMaterial.B1;
  coeff[1] = bestMaterial.B2;
  coeff[2] = bestMaterial.B3;
  coeff[3] = bestMaterial.C1;
  coeff[4] = bestMaterial.C2;
  coeff[5] = bestMaterial.C3;
    
  if (print_match) 
    printf("[OpticalMaterial by Abbe number] Best match for nd=%5.3f, vd=%5.2f: %s %s (nd=%5.3f, vd=%5.2f)\n",
	   nd,vd,bestMaterial.mfg, bestMaterial.name,bestMaterial.nd,bestMaterial.vd);

  return true;
};


OpticalMaterial::OpticalMaterial(const char* query, bool print_match)
{
    if(!init(query, print_match)) exit(-1);
}

bool OpticalMaterial::init(const char* query, bool print_match)
{
  char name[32];
  int j = 0;
  // Munch blanks in query, and convert to upper case:

  for (int i = 0; query[i]; ++i) {
    if (query[i]==' ') {}
    else if (query[i] >= 'a' && query[i] <= 'z') {
      name[j++] = query[i] + (int)'A' - (int)'a';
    } else {
      name[j++] = query[i];
    }
  }
  name[j] = 0;

  // user actually requested to not use the material, but ior/abbe, so fail and let the other method try:
  if(!strcmp(name, "ABBE")) return false;
    
  int hit_index = -1;
  float hit_quality = 0.f;

  for (int i = 0; i < NUM_MATERIALS; ++i) 
    if (strstr(glass_database[i].name, name)) {
      float quality = strlen(name) / (float) strlen(glass_database[i].name);
      if (quality > hit_quality) {
	hit_index = i;
	hit_quality = quality;
      }
    }
    
  if (hit_index >= 0) { // Partial match found!

    // If quality of match is not 100%, check for a better name match in the historical database.

    if (hit_quality < 0.99) {
      float alias_hit_quality = hit_quality;
      int alias_hit_index = -1;
      for (int i = 0; i < NUM_ALIASES; ++i) 
	if (strstr(alias_database[i].name, name)) {
	  float quality = strlen(name) / (float) strlen(alias_database[i].name);
	  if (quality > alias_hit_quality) {
	    alias_hit_index = i;
	    alias_hit_quality = quality;
	  }
	}
      
      // Test if historic material actually fits the name better
      if (alias_hit_quality > hit_quality) { 
	GlassAlias alias = alias_database[alias_hit_index];
	if (print_match) 
	  printf("[OpticalMaterial by name] Better match for \"%s\" found in historical database: \n"
		 "                          %s %s (%2.0f%%). Looking up closest Sellmeier material:\n",
		 query, alias.mfg, alias.name, 100*alias_hit_quality);
	
	float nd = alias.nd;
	float vd = alias.vd;
	OpticalMaterial abbe_material(nd, vd, print_match);
	for (int i = 0; i < 6; ++i)
	  coeff[i] = abbe_material.coeff[i];
	model = abbe_material.model;
	return true;
      }

    }

    // Else return best hit
    GlassDef hit = glass_database[hit_index];
    model = hit.model;
    coeff[0] = hit.B1;
    coeff[1] = hit.B2;
    coeff[2] = hit.B3;
    coeff[3] = hit.C1;
    coeff[4] = hit.C2;
    coeff[5] = hit.C3;
    if (print_match) 
      printf("[OpticalMaterial by name] Best match for \"%s\": %s %s (%2.0f%%)\n",
	     query,hit.mfg, hit.name, 100*hit_quality);
    return true;
  }


  if(print_match)
    printf("\x1B[31mERROR: [OpticalMaterial by name] No match found for query \"%s\"\n\x1B[0m",query);
  return false;
};

float OpticalMaterial::get_index(float lambda) {

  switch(model) {

  case mod_sellmeier:
    lambda = lambda * 0.001;
    return sqrt(coeff[0] * lambda * lambda/(lambda*lambda - coeff[3]) 
		+ coeff[1] * lambda * lambda/(lambda*lambda - coeff[4])
		+ coeff[2] * lambda * lambda/(lambda*lambda - coeff[5])
		+ 1);
    break;

  default:
    return -1;
    break;

  }

};