# $ICPRO_DIR/.icpro.login
# (jus) 2005/09/14
#
# This file will be sourced from tcsh only once when first 
# going into the ICPROJECT.
# - specific project module loads 
# - NOT for alias settings (put them into $ICPRO_DIR/.icpro.cshrc)
#
# Project:    %pproj% 
# Subproject: %sproj%

# use exact module version number (xx/n.n) to ensure reproducible 
# behaviour if modules default version is changed

#####################################################
if (-e .modules.$ICPRO_GROUP) then
  source .modules.$ICPRO_GROUP
endif
#####################################################

#####################################################
if ( -e ./env/user/$USER/.icpro_user.login ) then
  source ./env/user/$USER/.icpro_user.login
endif
