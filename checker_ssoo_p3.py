'''

Sistemas Operativos - Practica 3 - Programación de un intérprete de comandos (Uc3mshell)

Este programa verifica que el formato del entregable de la practica es el correcto (sigue las especificaciones de nombrado, y esta bien comprimido).

#### English version

Operating Systems - Laboratory 3 - Programming an Uc3mshell

This program verifies that the format of the laboratory assignment is correct (follows naming specifications and is properly compressed).

'''
import subprocess
import os
import sys
import shutil

practice_number = 3
authores_file_name = "autores.txt"

def print_format():
    # Añadimos la 'f' antes de las comillas para que reconozca las variables
    print(f"[ESP] Formato esperado para zip: ssoo_p{practice_number}_AAAAA_BBBBB_CCCCC.zip")
    print(f"[ESP] Formato esperado para {authores_file_name}: NIA1,Apellidos,Nombre(s)")
    print(f"[ENG] Expected format for zip: ssoo_p{practice_number}_AAAAA_BBBBB_CCCCC.zip")
    print(f"[ENG] Expected format for {authores_file_name}: NIA1,LastNames,Name(s)")


if __name__ == "__main__":
    '''
    Main function of the application. Gets the file passed as an argument. Decompresses it, checks its format and finally runs the tests.
    '''

    if shutil.which("unzip") is None:
        print("Error: The 'unzip' command is not installed on the system.")
        print("Error: El comando 'unzip' no está instalado en el sistema.")
        sys.exit(1)

    # We check that a file has been passed as an argument
    if not len(sys.argv) == 2:
        print('Uso / Usage: python {} <zip file>'.format(sys.argv[0]))
        print_format()
    else:
        print('CHECKER: comprobando / verifying', sys.argv[1])
        inputFile = sys.argv[1]
        
        # We check that the file exists
        if not os.path.isfile(inputFile):
            print("The file", inputFile, "does not exist")
            sys.exit(0)
    
        # We check the file name format
        tokens=inputFile.replace(".zip","")
        tokens=tokens.split("_")
        if len(tokens) != 3 and len(tokens) != 4 and len(tokens) != 5:
            print("Formato de nombre de archivo incorrecto / Incorrect file name format")
            print_format()
            sys.exit(0)
            
        ssoo=tokens[0]
        p1=tokens[1]
        u1=tokens[2]
        u2=""
        u3=""
        if len(tokens)>3:
            u2=tokens[3]
        if len(tokens)>4:
            u3=tokens[4]
        if not (ssoo == "ssoo" and p1 == f"p{practice_number}"):
            print("Formato de nombre de archivo incorrecto / Incorrect file name format")
            print_format()
            sys.exit(0)

        print("CHECKER: NIA 1:", u1, "NIA 2:", u2, "NIA 3:", u3)
        print('CHECKER: descomprimiendo / decompressing')

        tempFolder="./check"

        # Check if the temporal folder exists BEFORE creating it
        if os.path.exists(tempFolder):
            print(f"El directorio '{tempFolder}' ya existe. / Directory '{tempFolder}' already exist.")
            confirmacion = input("¿Deseas eliminarlo para continuar? / Would you like to delete it to continue? (y/n): ").lower()
            
            if confirmacion == 'y':
                try:
                    shutil.rmtree(tempFolder)
                    print(f"CHECKER: Directorio '{tempFolder}' eliminado. /  Directory '{tempFolder}' deleted.")
                except Exception as e:
                    print(f"Error al intentar eliminar el directorio / Error while trying to delete the directory: {e}")
                    sys.exit(1)
            else:
                print("Operación cancelada. / Operation canceled.")
                sys.exit(0)

        # Ahora sí, creamos el directorio
        subprocess.call(["mkdir", "-p", tempFolder])

        zipresult=subprocess.call(["unzip","-j",inputFile,"-d",tempFolder])
        if not zipresult == 0:
            print("Error al descomprimir el archivo zip. / Error decompressing the zip file.")
            sys.exit(0)

        # We check that the authors file exists
        autoresPath = f"{tempFolder}/{authores_file_name}"
        if not os.path.isfile(autoresPath):
            print(f"El archivo «{authores_file_name}» no existe en la carpeta descomprimida /")
            print(f"The file {authores_file_name} does not exist in the decompressed folder")
            sys.exit(0)
    
        print(f"CHECKER: comprobando el formato de {authores_file_name} / verifying format of {authores_file_name}")
        try:
            with open(autoresPath, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()
                
                if not lines:
                    print(f"Error: {authores_file_name} esta vacío. / {authores_file_name} is empty.")
                    sys.exit(0)

                for linea_num, line in enumerate(lines, 1):
                    line = line.strip()
                    if not line: 
                        continue 

                    parts = line.split(",")
                    print("Línea / Line {}: {}".format(linea_num, line))
                    
                    if len(parts) != 3:
                        print(f"Error en el formato de {authores_file_name} / Error in the format of {authores_file_name} (Línea / Line {linea_num}): '{line}'")
                        print_format()
                        sys.exit(0)
                    
                    nia, apellidos, nombre = parts[0].strip(), parts[1].strip(), parts[2].strip()

                    # Verify that NIA is a number
                    if not nia.isdigit():
                        print(f"Error en el formato de {authores_file_name} / Error in the format of {authores_file_name} (Línea / Line {linea_num}): La NIA '{nia}' no es válida / The NIA '{nia}' is not valid.")
                        print_format()
                        sys.exit(0)

                    # Verify that last names and name have content
                    if not apellidos or not nombre:
                        print(f"Error en el formato de {authores_file_name} / Error in the format of {authores_file_name} (Line {linea_num}): Faltan los apellidos o el nombre / Missing last names or name.")
                        print_format()
                        sys.exit(0)

        except Exception as e:
            print(f"Error al leer {authores_file_name} / Error reading {authores_file_name}: {e}")
            sys.exit(0)

        # Compile
        makeres=subprocess.call(["make"], cwd=tempFolder)
        if not makeres == 0:
            print("[-] Error de compilación / Compilation error")
            sys.exit(0)
        else:
            print("[+] Compilación correcta / Compilation successful")