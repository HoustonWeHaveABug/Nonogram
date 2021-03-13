function nonogram_org_conv() {
	var H = [], D = d[1][0]%d[1][3]+d[1][1]%d[1][3]-d[1][2]%d[1][3], C = d[2][0]%d[2][3]+d[2][1]%d[2][3]-d[2][2]%d[2][3], Aa = d[3][0]%d[3][3]+d[3][1]%d[3][3]-d[3][2]%d[3][3], N = [], O = [], E = [];
	for (x = 5; x < Aa+5; x++) {
		var Ea = d[x][0]-d[4][1], Fa = d[x][1]-d[4][0], Ga = d[x][2]-d[4][3];
		z = d[x][3]-Ea-d[4][2];
		H[x-5] = [ (Ea+256).toString(16).substring(1)+((Fa+256<<8)+Ga).toString(16).substring(1), z ];
	}
	for (w = 0; w < C; w++) {
		for (E[w] = [], v = 0; v < D; v++) {
			E[w][v] = 0;
		}
	}
	var V = Aa+5, Ha = d[V][0]%d[V][3]*(d[V][0]%d[V][3])+d[V][1]%d[V][3]*2+d[V][2]%d[V][3], Ia = d[V+1];
	for (x = V+2; x <= V+1+Ha; x++) {
		for (v = d[x][0]-Ia[0]-1; v < d[x][0]-Ia[0]+d[x][1]-Ia[1]-1; v++) {
			E[d[x][3]-Ia[3]-1][v] = d[x][2]-Ia[2];
		}
	}
	var W = Aa+7+Ha;
	if (d.length > W) {
		var Ka = [];
		for (w = 0; w < C; w++) {
			for (Ka[w] = [], v = 0; v < D; v++) {
				Ka[w][v] = 0;
			}
		}
		var La = d[W][0]%d[W][3]*(d[W][0]%d[W][3])+d[W][1]%d[W][3]*2+d[W][2]%d[W][3], Ma = d[W+1];
		for (x = W+2; x <= W+1+La; x++) {
			for (v = d[x][0]-Ma[0]-1; v < d[x][0]-Ma[0]+d[x][1]-Ma[1]-1; v++) {
				Ka[d[x][3]-Ma[3]-1][v] = d[x][2]-Ma[2];
			}
		}
	}
	for (w = 0; w < C; w++) {
		N[w] = [];
		for (v = 0; v < D; ) {
			var Na = v;
			for (z = E[w][v]; v < D && E[w][v] == z; ) {
				v++;
			}
			0 < v-Na && 0 < z && (N[w][N[w].length] = [v-Na,z]);
		}
	}
	for (v = 0; v < D; v++) {
		O[v] = [];
		for (w = 0; w < C; ) {
			var Oa = w;
			for (z = E[w][v]; w < C && E[w][v] == z; ) {
				w++;
			}
			0 < w-Oa && 0 < z && (O[v][O[v].length] = [ w-Oa, z ]);
		}
	}
	var X = '<p>'+D+'<br/>'+C+'<br/>'+(1 < H.length ? '1':'0')+'<br/>';
	for (w = 0; w < D; w++) {
		X += w > 0 ? ',':'';
		X += '"';
		if (O[w].length > 0) {
			for (y in O[w]) {
				X += y > 0 ? ',':'';
				X += ''+O[w][y][0]+(1 < H.length ? '-'+(O[w][y][1]-1):'');
			}
		}
		else {
			X += '0';
		}
		X += '"';
	}
	X += '<br/>';
	for (w = 0; w < C; w++) {
		X += w > 0 ? ',':'';
		X += '"';
		if (N[w].length > 0) {
			for (y in N[w]) {
				X += y > 0 ? ',':'';
				X += ''+N[w][y][0]+(1 < H.length ? '-'+(N[w][y][1]-1):'');
			}
		}
		else {
			X += '0';
		}
		X += '"';
	}
	X += '</p>';
	return X;
}
