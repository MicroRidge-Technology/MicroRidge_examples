
#if 0
VIV_DIR=$1
SIM_DIR=$2
SIM_LIB=$3
exec_file=$(mktemp -p .)
c++  -Wl,--disable-new-dtags  $0 -o $exec_file -I$VIV_DIR/data/xsim/include -L$VIV_DIR/lib/lnx64.o   -L$SIM_DIR -Wl,-rpath,$VIV_DIR/lib/lnx64.o:$SIM_DIR  -lxv_simulator_kernel  -l${SIM_LIB}_xsim
$exec_file $4 ${SIM_LIB}
ret=$?
rm $exec_file
exit $ret
#endif

#include <stdio.h>
#include <xsi.h>

int main(int argc, char **argv) {
  s_xsi_setup_info info = {0};
  xsiHandle xsi_handle = xsi_open(&info);
  if (argc < 3) {
    fprintf(stderr, "Usage %s outfile.hpp struct_typename\n", argv[0]);
    return 1;
  }
  const char *out_filename = argv[1];
  const char *struct_typename = argv[2];

  FILE *outf = fopen(out_filename, "w+");
  fprintf(outf, "#ifndef __%s__HEADER\n", struct_typename);
  fprintf(outf, "#define __%s__HEADER\n", struct_typename);
  fprintf(outf, "#include \"xsim_driver.hpp\"\n");
  fprintf(outf, "#include <filesystem>\n");
  fprintf(outf, "struct %s {\n", struct_typename);

  int num_ports = xsi_get_int(xsi_handle, xsiNumTopPorts);
  for (int p = 0; p < num_ports; ++p) {
    const char *port_name = xsi_get_port_name(xsi_handle, p);
    int width = xsi_get_int_port(xsi_handle, p, xsiHDLValueSize);
    fprintf(outf, "\tsim_port<%d> %s;\n", width, port_name);
  }
  fprintf(outf, "\t%s(xsiHandle handle):\n", struct_typename);
  for (int p = 0; p < num_ports; ++p) {
    const char *port_name = xsi_get_port_name(xsi_handle, p);
    fprintf(outf, "\t\t%s(handle,\"%s\")", port_name, port_name);
    if (p != num_ports - 1) {
      fprintf(outf, ",");
    }
    fprintf(outf, "\n");
  }
  fprintf(outf, "\t{}\n");
  fprintf(outf, "\tstatic std::filesystem::path get_sim_dir(){\n");
  fprintf(outf, "\t\tauto pth = std::filesystem::path(__FILE__);\n");
  fprintf(outf, "\t\treturn pth.remove_filename() / \"..\" / \"..\";\n");
  fprintf(outf, "\t}\n");
  fprintf(outf, "};\n");
  fprintf(outf, "#endif\n");
}
