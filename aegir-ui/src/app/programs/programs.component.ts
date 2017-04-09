import { Component, OnInit } from '@angular/core';
import { Program } from '../program';
import { ApiService } from '../api.service';

@Component({
    selector: 'app-programs',
    templateUrl: './programs.component.html',
    styleUrls: ['./programs.component.css'],
    providers: [ ApiService ]
})
export class ProgramsComponent implements OnInit {
    errorMessage: string;
    programs: Program[];

    constructor(private apiService: ApiService) { }

    ngOnInit() {
	this.getPrograms();
    }

    getPrograms() {
	this.apiService.getPrograms()
	    .subscribe(
		programs => this.programs = programs,
	    error => this.errorMessage = <any>error);
    }

}
