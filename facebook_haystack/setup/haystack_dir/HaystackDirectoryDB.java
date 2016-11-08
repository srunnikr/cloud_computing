public class HaystackDirectoryDB{
	
	public void createTable(String[] parameters){
		String query = "CREATE TABLE photos(photo_id int PRIMARY KEY, "
         + "cookie text, "
         + "logical_volume_id varint, "
         + "alt_key varint );";
		
      //Creating Cluster object
      Cluster cluster = Cluster.builder().addContactPoint("127.0.0.1").build();
   
      //Creating Session object
      Session session = cluster.connect("cse291");
 
      //Executing the query
      session.execute(query);
 
      System.out.println("Table created");
	}
	
	public void add_mapping(String[] parameters){
		//create cookie
		String m_cookie = "example";
		//create key and alternate key
		Integer m_alt_key = dosomething(parameters[0]);
		//find logical volume ID
		//TODO
		lvi = getLogicalVolumeId(); 
		String query1 = "INSERT INTO emp (photo_id, cookie, logical_volume_id, alt_key)"
         + " VALUES(parameters[0],m_cookie, lvi, m_alt_key,);" ;		 
		//Creating Cluster object
      Cluster cluster = Cluster.builder().addContactPoint("127.0.0.1").build();
 
      //Creating Session object
      Session session = cluster.connect("cse291");
       
      //Executing the query
      session.execute(query1);
	  
	}
	
	public void remove_mapping(String[] parameters){
		//create cookie
		String m_cookie = "example";
		//create key and alternate key
		Integer m_alt_key = dosomething(parameters[0]);
		//find logical volume ID
		//TODO
		lvi = getLogicalVolumeId(); 
		String query1 = "DELETE FROM photos WHERE photo_id="+parameters[0]+";" ;		 
		//Creating Cluster object
      Cluster cluster = Cluster.builder().addContactPoint("127.0.0.1").build();
 
      //Creating Session object
      Session session = cluster.connect("cse291");
       
      //Executing the query
      session.execute(query1);
	  
	}
	
	public static void main(String[] args){
		
		Cluster cluster;
		Session session;
		
		cluster = cluster.builder().addContactPoint("127.0.0.1").build();
		session = cluster.connect();
		
		String query = "CREATE KEYSPACE cse291 WITH replication "+ "= {'class':'SimpleStrategy', 'replication_factor':1}; ";
		session.execute(query);
		session.execute("USE cse291");
		
		
	}
	
}