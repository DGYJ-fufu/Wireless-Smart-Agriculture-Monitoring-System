import './App.css'
import axios from 'axios'
import { useEffect, useState } from 'react'

interface Students{
  学号: string;
  姓名: string;
  性别: string;
  班级号: string;
  登陆时间: string;
}

const APT_URL = "http://localhost:5555/api/students";

function App() {
  const [readStudents, setReadStudents] = useState<Students[]>([]);
  useEffect(() => {
    const getStudents = async () => {
      const resquest = await axios.get(APT_URL);
      readStudents(resquest);
    }
  getStudents();
  },[]);
  return(
    <div>
      <ul>
        {readStudents.map(
          <li key = {readStudents.学号}>
            {readStudents.姓名} +++ {readStudents.性别} +++ {readStudents.班级号} +++ {readStudents.登陆时间}
          </li>
        )}
      </ul>
    </div>
  )
}

export default App
