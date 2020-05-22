#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sqlite3.h>

void add_package(sqlite3 *projs_db, char *pkg_name, char *dwnld_cmd )
{

	int rc, step;
	sqlite3_stmt *res;

	rc = sqlite3_prepare_v2(projs_db, "Insert into project (name, command) values (?,?);", -1, &res, 0 );

	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(projs_db) );
		exit(EXIT_FAILURE);
	}

	sqlite3_bind_text(res, 1, pkg_name, -1, NULL );
	sqlite3_bind_text(res, 2, dwnld_cmd, -1, NULL );

	step = sqlite3_step(res);

}

char *search_package(sqlite3 *projs_db, char *pkg_name )
{
	int rc, step;
	char *command, *retreived;
	sqlite3_stmt *res;

	rc = sqlite3_prepare_v2(projs_db, "Select command from project where name = ?;", -1, &res, 0 );

	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(projs_db) );
		exit(EXIT_FAILURE);
	}

	sqlite3_bind_text(res, 1, pkg_name, -1, NULL );

	rc = sqlite3_step(res);

	if( rc == SQLITE_ROW )
	{
		retreived = sqlite3_column_text(res, 0);
		command = malloc( strlen(retreived) + 1 );
		strcpy(command, retreived );
		printf("pkg download command: %s\n", retreived );
		sqlite3_finalize(res);
		return command;
	}
	else
	{
		sqlite3_finalize(res);
		return NULL;
	}
}

void sync_package(sqlite3 *projs_db, char *pkg_name, char *projpack_path, int chdir_flag )
{
	struct stat sb;
	if(chdir_flag)
	{
		chdir(projpack_path);
		chdir("projs");
	}
	if( stat(pkg_name, &sb ) == 0 && S_ISDIR(sb.st_mode) )
	{
		chdir(pkg_name);
		chmod(".projpack/update", S_IXUSR | S_IRUSR  );
		system(".projpack/update");
	}
	else
	{
		system( search_package(projs_db, pkg_name )  );
		chdir(pkg_name);
		FILE *depsfile;
		char depname[128];
		depsfile = fopen(".projpack/deps.list", "r" );
		if( depsfile == NULL )
		{
			fprintf(stderr, "Error while processing dependencies of pkg: %s\n", pkg_name );
			exit(EXIT_FAILURE);
		}
		while( fscanf(depsfile, "%127s", depname ) != EOF )
			sync_package(projs_db, depname, projpack_path, 0 );
		fclose(depsfile);
		chmod(".projpack/install", S_IXUSR | S_IRUSR  );
		system(".projpack/install");
	}
}

int main(int argc, char **argv )
{

	int opt, rc, step, add_flag = 0, add_list_flag = 0, sync_flag = 0;
	char *pkg_name, *dwnld_cmd, *home_path, *projpack_path, *db_path, *list_path, *sync_name, *sql_err = NULL;
	sqlite3 *projs_db;
	sqlite3_stmt *res;

	home_path = getenv("HOME");
	db_path = malloc( strlen(home_path) + strlen("/.projpack/projs.db") + 1  );
	projpack_path = malloc( strlen(home_path) + strlen("/.projpack") + 1  );
	strcpy(db_path, home_path );
	strcpy(projpack_path, home_path );
	strcat(db_path, "/.projpack/projs.db" );
	strcat(projpack_path, "/.projpack" );

	rc = sqlite3_open(db_path, &projs_db );
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "Error: could not open projects database. Try mkdir ~/.projpack \n" );
		exit(EXIT_FAILURE);
	}

	rc = sqlite3_exec(projs_db, "Create table if not exists project( name TEXT NOT NULL UNIQUE, command TEXT NOT NULL );", 0, 0, &sql_err );

	while(  ( opt = getopt(argc, argv, "a:S:A:" ) )  !=  -1  )
	{
		switch(opt)
		{
			case 'a':

				pkg_name = optarg;
				dwnld_cmd = strchr(pkg_name, ' ' );

				if( dwnld_cmd == NULL )
				{
					fprintf(stderr, "Error: expected command after package name using -a option.\n" );
					exit(EXIT_FAILURE);
				}

				*(dwnld_cmd++) = 0;

				add_flag = 1;
	
				printf("db_path: %s\n", db_path ); 
				printf("adding package %s with download command %s\n", pkg_name, dwnld_cmd );
				
				break;

			case 'S':
				sync_flag = 1;
				sync_name = optarg;
				printf("syncing package %s\n", optarg );
				break;

			case 'A':
				add_list_flag = 1;
				printf("url list file: %s\n", optarg );
				break;

		}
	}

	if( add_flag )
	{
		add_package(projs_db, pkg_name, dwnld_cmd );
	}
	if( add_list_flag )
	{
	}
	if( sync_flag )
		sync_package(projs_db, sync_name, projpack_path, 1 );
	return 0;
}
