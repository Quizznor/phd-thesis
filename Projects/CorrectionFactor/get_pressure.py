import mysql.connector
from datetime import timedelta, date
from getpass import getpass
from pathlib import Path

start_dt = date(2025, 1, 1)
end_dt = date(2025, 1, 5)

def daterange(date1, date2):
    for n in range(int ((date2 - date1).days)+1):
        yield date1 + timedelta(n)

def get_E_field_data():
    stations =  ['CRS']
    TableName = {'CRS': 'Weather'}
    Columns = {'CRS': 'TimeStamp, Pressure, Temperature'}

    basepath = '/cr/data01/filip/Data/AeraPressureTemperature/'

    for dt in daterange(start_dt, end_dt):
        print('')
        print(dt)
        year = dt.year
        month = dt.month
        day = dt.day

        for station in stations:
            print(f'Fetching pressure data from station {station}')

            pressure_file_dir = basepath + f'{station}/{year}/{month:02d}'
            pressure_file_path = pressure_file_dir + f'/{station}_{year}_{month:02d}_{day:02d}.dat'
            # print(pressure_file_path)
            Path(pressure_file_dir).mkdir(parents=True, exist_ok=True)

            command = f"select {Columns[station]} from {TableName[station]} where TimeStamp like '{dt}%' order by TimeStamp asc;"

            # print(command)

            cursor.execute(command)
            data = cursor.fetchall()
            if len(data) == 0:
                print(f'No data found from station {station}.')
                continue
            print(len(data), 'entries')

            with open(pressure_file_path, 'w') as file:
                # writer = csv.writer(file, lineterminator='\n')
                # file.write(Columns[station] + '\n')
                for row in data:
                    # Convert the datetime object to a string before writing
                    file.write(f"{int(row[0].timestamp())} " + " ".join([str(i) for i in row[1:]]) + "\n")


            # dates = np.array(data)[:,0]
            # pressure = np.array(data)[:,1]




if __name__ == "__main__":
    # setup connection to mon DB
    host = 'mondb.auger.uni-wuppertal.de'
    usr = 'AugerPhase2'
    db = 'AERA'
    print('Connecting to ' + host + ' Database ' + db +' \n as ' + usr)
    pw = getpass()
    mc = mysql.connector.connect(host=host,user=usr,password=pw,database=db,auth_plugin='mysql_native_password')
    cursor = mc.cursor()

    get_E_field_data()
