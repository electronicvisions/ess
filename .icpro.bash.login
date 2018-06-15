# $ICPRO_DIR/.icpro.bash.login
# (md) 2006/07/07
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
if [ -f .modules.$ICPRO_GROUP ]; then
  source .modules.$ICPRO_GROUP
fi
#####################################################

#####################################################
if [ -f ./env/user/$USER/.icpro_user.bash.login ]; then
  source ./env/user/$USER/.icpro_user.bash.login
fi


