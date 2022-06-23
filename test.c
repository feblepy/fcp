#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

//variable
typedef enum{
	unknow=0,
	txt=1,
	bin=2
}file_type_t;
typedef enum{
	def=1,
	bigend=1,
	littlend=2
}byte_order_t;
typedef struct{
	union{
	uint8_t _reg_cnt[1024];
	uint32_t reg_cnt[256];//reg
	};
	file_type_t file_type;//type file
	byte_order_t byte_order;
	char file_name[34];//file
	char dummy;
	char file_name_len;//lenght file
	char file_name_presuff;//lenght file without suffix
}flag_t;

flag_t flag_msg = {0};

const char helper[] = "\
File conversion program txt->bin or bin->txt with code crc32 and create new file\n\
Text file content \"0,f,f,f,0,0,0,5\" etc.\n\
One string - one register\n\
\n\
Usage: fcp file.txt or file.bin\n\
Options:\n\
  -h\t\t\tsee this help and exit\n\
  Output file byte order:\n\
  -b\t\t\tbig endian(default)\n\
  -l\t\t\tlittle endian\n";

const char no_file_arg[] = "\
fcp: no unput file\n\
for info use flag -h\n";

const char unknargs[] = "\
fcp: unknow arguments\n\
for info use flag -h\n";

//prototyp func
size_t read_file(FILE* f,unsigned char *ch_r,uint32_t SIZE), write_file(FILE* f,unsigned char* ch_r,uint32_t SIZE);
uint32_t crc32(uint32_t* from, uint32_t amountreg);
void reverse();
unsigned char bin_to_char(unsigned char tmp);

void print_exit(const char* msg){
	printf(msg);
	exit(1);
}

void parse(int argc,char** argv){
	int i,j;
	int flag_args=0;
	int flag_file=0;
	int flag_dot=0;
	
	if(argc==1){
		print_exit(no_file_arg);
	}
	for(i=1; i<argc; i++){
		if(argv[i][0] == '-') flag_args=1;
		else flag_file=1;
		for(j=0; argv[i][j]!='\0'; j++){ 
			if(flag_args && argv[i][j] != '\0')
				switch(argv[i][++j]){
				case 'h': print_exit(helper); break;
				case 'b': flag_msg.byte_order=bigend; break;
				case 'l': flag_msg.byte_order=littlend; break;
				default: print_exit(unknargs);
			}
			if(flag_file && argv[i][j] != '\0'){
				flag_msg.file_name[j]=argv[i][j];
				if(flag_dot){
					switch(argv[i][j]){
					case 't': flag_msg.file_type=txt; break;
					case 'b': flag_msg.file_type=bin; break;
					default: flag_msg.file_type=unknow; break;
					}
					flag_dot=0;
				}
				if(argv[i][j] == '.'){
					flag_dot=1;
					flag_msg.file_name_presuff = j;
				}
			}
		}
		if(flag_file){
			flag_msg.file_name[++j]='\0';
			flag_msg.file_name_len = j;
		}
		flag_file=0;
		flag_args=0;
	}
}

void copy_mas(unsigned char* from,unsigned char* to, uint8_t posfrom){
	uint8_t i=posfrom,y=0;
	while((to[i]=from[y])!='\0'){
		++i;
		++y;
	}
}

//main
int main(int argc, char** argv){
	FILE* file;
	uint32_t SIZE = 4590;
	unsigned char ch_r[4590] = {0};
	
	parse(argc, argv);
	if(!read_file(file,ch_r,SIZE))
		printf("file %s read\n",flag_msg.file_name);	
	if(!write_file(file,ch_r,SIZE)){
		printf("file %s create\n",flag_msg.file_name);
		if(flag_msg.file_type == 2){
			size_t i,j,tmp;
			
			printf("crc= 0x");
			for(i=28,j=4590,tmp=0;j<4598;j++,i-=4){
				tmp=((unsigned char)(flag_msg.reg_cnt[255]>>i))&0x0f;
				ch_r[j]=(bin_to_char(tmp));
				printf("%c",ch_r[j]);
			}
		}
	}
	return 0;
}

void reverse(){
	uint8_t tmp;
	size_t i,j,k;
	
	for(i=0,j=3,k=j;j<=1020;i+=2,j+=6,k=j){
		for(;i<k-1;i++,j--){
			tmp=flag_msg._reg_cnt[i];
			flag_msg._reg_cnt[i]=flag_msg._reg_cnt[j];
			flag_msg._reg_cnt[j]=tmp;
		}
	}
}

unsigned char bin_to_char(unsigned char tmp){
	if(tmp>=0 && tmp<=9)
		return (tmp + (uint8_t)48);
	if(tmp>=10 && tmp<=15);
		return (tmp + (uint8_t)87);
}

size_t read_file(FILE* f,unsigned char* ch_r,uint32_t SIZE){
	size_t i,j,k,result;
	char file[flag_msg.file_name_len];
	copy_mas(flag_msg.file_name,file,0);

	char* r_bin= "r+b";
	char* r_txt= "r+";
	char r_accees[5];
	void* ptr_r = (void*)ch_r;
	size_t qout_mark=0;
	size_t endstr_mark=0;
	size_t dot_cnt=0;
	
	if(flag_msg.file_type == txt){
		copy_mas(r_txt,r_accees,0);
		f = fopen(file, r_accees);
		fread(ptr_r,sizeof(unsigned char),SIZE,f);
		for(i=0,j=0; i<SIZE; i++){
			ch_r[i] = ((unsigned char*)ptr_r)[i];
			
			if(isspace(ch_r[i]) || ispunct(ch_r[i]))
				switch(ch_r[i]){
				case '"': //qout_mark++; 
				break;
				case ',': dot_cnt++;
				break;
				case '\n':
					j++;
					endstr_mark++;
				break;
				case '\0':
				break;
				}

			if(isalnum(ch_r[i])){
				if(dot_cnt>0 && dot_cnt<=7){ 
					flag_msg.reg_cnt[j]=flag_msg.reg_cnt[j]<<(uint32_t)4;
					if(dot_cnt==7){
					dot_cnt=0;
					}
				}
				if(isdigit(ch_r[i])){
					if(ch_r[i]>='0' && ch_r[i]<='9'){
						flag_msg.reg_cnt[j]|=ch_r[i]-(uint8_t)48;
					}
				}
				else{
					if(ch_r[i]>='a' && ch_r[i]<='f'){
						flag_msg.reg_cnt[j]|=ch_r[i]-(uint8_t)87;
					} else 
					if(ch_r[i]>='A' && ch_r[i]<='F')
						flag_msg.reg_cnt[j]|=ch_r[i]-(uint8_t)55;
					else{
						printf("error unknow character %c in line %d, pos %d",ch_r[i],endstr_mark,i-endstr_mark*18,ch_r[i]);
						fclose(f);
						exit(1);
					}
				}
			}
		}
		if(flag_msg.byte_order == littlend)
			reverse();
		flag_msg.reg_cnt[255] = crc32(flag_msg.reg_cnt,255);
		}
	else if(flag_msg.file_type == bin){
		copy_mas(r_bin,r_accees,0);
		unsigned char tmp,shift;
		ptr_r = (void*)flag_msg._reg_cnt;
		f = fopen(file, r_accees);
		fread(ptr_r,sizeof(unsigned char),1024,f);
		
			for(i=0,j=0; i<256 && j<SIZE; i++ ){
				flag_msg.reg_cnt[i]=((unsigned int*)ptr_r)[i];
				//reverse();
					if(i==255){
						printf("break- %d\n",flag_msg.reg_cnt[i]);
						break;
					}
					for(k=0,shift=28;k<18 && j<SIZE;j++,k++){
						switch(k){
							case 0: case 16:
								ch_r[j]='"';
								break;
							case 17:
								ch_r[j]='\n'; 
								break;
							default:
								if((k%2)){
									tmp=flag_msg.reg_cnt[i]>>shift;
									tmp=tmp&0x0f;
									ch_r[j] = bin_to_char(tmp);
									shift-=4;
									}
								if(!(k%2)){
									ch_r[j] = ','; 
								}
							break;
						}
					}
				}
				
			for(i=28,j=4590;j<4598;j++,i-=4){
				tmp=((unsigned char)(flag_msg.reg_cnt[255]>>i))&0x0f;
				ch_r[j]=(bin_to_char(tmp));
				}
		}
	else 
		print_exit(no_file_arg);
	
	result=fclose(f);
}

size_t write_file(FILE* f,unsigned char* ch_r,uint32_t SIZE){
	size_t i;
	char* bin= ".bin";
	char* txt= ".txt";
	const char* w_bin= "w+b";
	const char* w_txt= "w+";
	char file[flag_msg.file_name_len];
	copy_mas(flag_msg.file_name,file,0);
	void *ptr_wr;

	if(flag_msg.file_type == 2){
		copy_mas(txt,file,flag_msg.file_name_presuff);
		f=fopen(file, w_txt);
		SIZE = 4598;
		ptr_wr = (void*)ch_r;
	}
	else{
		copy_mas(bin,file,flag_msg.file_name_presuff);
		f=fopen(file, w_bin);
		SIZE=1024;
		ptr_wr = (void*)flag_msg._reg_cnt;
	}
	fwrite(ptr_wr,sizeof(char),SIZE,f);
	i=fclose(f);
	copy_mas(file,flag_msg.file_name,0);
	return i;
}

uint32_t crc32(uint32_t* from, uint32_t amountreg){
	uint32_t i,adress_cnt,size_reg,crc32;
	const uint32_t poly=0x4c11db7;
	const uint32_t mask=0x80000000;
	
	size_reg=sizeof(from)*8;
	for(adress_cnt=0,crc32=0xffffffff; adress_cnt<amountreg; adress_cnt++){
		crc32^=from[adress_cnt];
		for(i=0; i<size_reg;i++){
			if(crc32&0x80000000) crc32=(crc32<<1)^poly;
			else crc32=crc32<<1;
		}
	}
	return crc32;
}